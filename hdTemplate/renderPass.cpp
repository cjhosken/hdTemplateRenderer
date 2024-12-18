//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderPassState.h"
#include "renderDelegate.h"
#include "renderPass.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderPass::HdTemplateRenderPass(HdRenderIndex *index,
                                       HdRprimCollection const &collection,
                                       HdRenderThread* renderThread,
                                       HdTemplateRenderer *renderer,
                                       std::atomic<int> *sceneVersion)
    : HdRenderPass(index, collection)
    , _renderThread(renderThread)
    , _renderer(renderer)
    , _sceneVersion(sceneVersion)
    , _lastSceneVersion(0)
    , _lastSettingsVersion(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _aovBindings()
    , _colorBuffer(SdfPath::EmptyPath())
{
    renderer->SetIndex(index);
}

HdTemplateRenderPass::~HdTemplateRenderPass()
{
    // Make sure the render thread's not running, in case it's writing
    // to _colorBuffer/_depthBuffer.
    _renderThread->StopRender();
}

bool
HdTemplateRenderPass::IsConverged() const {
    return false;
}

static
GfRect2i
_GetDataWindow(HdRenderPassStateSharedPtr const& renderPassState)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();
    if (framing.IsValid()) {
        return framing.dataWindow;
    } else {
        // For applications that use the old viewport API instead of
        // the new camera framing API.
        const GfVec4f vp = renderPassState->GetViewport();
        return GfRect2i(GfVec2i(0), int(vp[2]), int(vp[3]));        
    }
}

void
HdTemplateRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState,
                             TfTokenVector const &renderTags)
{
    // Determine whether the scene has changed since the last time we rendered.
    bool needStartRender = false;
    int currentSceneVersion = _sceneVersion->load();
    if (_lastSceneVersion != currentSceneVersion) {
        needStartRender = true;
        _lastSceneVersion = currentSceneVersion;
    }

    // Likewise the render settings.
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        _renderThread->StopRender();
        _lastSettingsVersion = currentSettingsVersion;

        //_renderer->SetRandomNumberSeed(
        //    renderDelegate->GetRenderSetting<unsigned int>(
        //        HdEmbreeRenderSettingsTokens->randomNumberSeed, (unsigned int)-1));

        needStartRender = true;
    }

    // Determine whether we need to update the renderer camera.
    const GfMatrix4d view = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    if (_viewMatrix != view || _projMatrix != proj) {
        _viewMatrix = view;
        _projMatrix = proj;

        _renderThread->StopRender();
        _renderer->SetCamera(_viewMatrix, _projMatrix);
        needStartRender = true;
    }

    // Check if the window has been resized

    const GfRect2i dataWindow = _GetDataWindow(renderPassState);

    if (_dataWindow != dataWindow) {
        _dataWindow = dataWindow;

        _renderThread->StopRender();
        _renderer->SetDataWindow(dataWindow);

        if (!renderPassState->GetFraming().IsValid()) {
            // Support clients that do not use the new framing API
            // and do not use AOVs.
            //
            // Note that we do not support the case of using the
            // new camera framing API without using AOVs.
            //
            const GfVec3i dimensions(_dataWindow.GetWidth(),
                                     _dataWindow.GetHeight(),
                                     1);

            _colorBuffer.Allocate(
                dimensions,
                HdFormatUNorm8Vec4,
                /*multiSampled=*/true);
        }

        needStartRender = true;
    }

    HdRenderPassAovBindingVector aovBindings =
        renderPassState->GetAovBindings();
    if (_aovBindings != aovBindings || _renderer->GetAovBindings().empty()) {
        _aovBindings = aovBindings;

        _renderThread->StopRender();
        if (aovBindings.empty()) {
            HdRenderPassAovBinding colorAov;
            colorAov.aovName = HdAovTokens->color;
            colorAov.renderBuffer = &_colorBuffer;
            colorAov.clearValue =
                VtValue(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
            aovBindings.push_back(colorAov);
        }
        _renderer->SetAovBindings(aovBindings);
        // In general, the render thread clears aov bindings, but make sure
        // they are cleared initially on this thread.
        _renderer->Clear();
        needStartRender = true;
    }

    // Only start a new render if something in the scene has changed.
    needStartRender = true;
    if (needStartRender) {
        _renderer->MarkAovBuffersUnconverged();
        _renderThread->StartRender();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE