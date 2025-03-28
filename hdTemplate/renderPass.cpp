#include "renderPass.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "renderDelegate.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderPass::HdTemplateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection, HdRenderThread *renderThread, HdTemplateRenderer *renderer, std::atomic<int> *sceneVersion)
    : HdRenderPass(index, collection)
    , _renderThread(renderThread)
    , _renderer(renderer)
    , _sceneVersion(sceneVersion)
    , _lastSceneVersion(0)
    , _lastSettingsVersion(0)
    , _colorBuffer(SdfPath::EmptyPath())
    , _depthBuffer(SdfPath::EmptyPath())
    , _converged(false)
{
    _renderer->SetScene(SceneData(index));
}

HdTemplateRenderPass::~HdTemplateRenderPass() {
    _renderThread->StopRender();
}

bool HdTemplateRenderPass::IsConverged() const {
    if (_aovBindings.size() == 0) {
        return _converged;
    }
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        if (_aovBindings[i].renderBuffer && !_aovBindings[i].renderBuffer->IsConverged()) {
            return false;
        }
    }
    return true;
}

static 
GfRect2i
_GetDataWindow(HdRenderPassStateSharedPtr const& renderPassState)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();
    if (framing.IsValid()) {
        return framing.dataWindow;
    } else {
        const GfVec4f vp = renderPassState->GetViewport();
        return GfRect2i(GfVec2i(0), int(vp[2]), int(vp[3]));
    }
}


void HdTemplateRenderPass::_Execute(HdRenderPassStateSharedPtr const &renderPassState,
                                    TfTokenVector const &renderTags)
{
    bool needStartRender = false;

    int currentSceneVersion = _sceneVersion->load();
    if (_lastSceneVersion != currentSceneVersion) {
        needStartRender = true;
        _renderer->BuildBVH();
        _lastSceneVersion = currentSceneVersion;
        
    }

    // Check in the camera has updated
    const GfMatrix4d view = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d proj = renderPassState->GetProjectionMatrix();
    if (_viewMatrix != view || _projMatrix != proj) {
        _viewMatrix = view;
        _projMatrix = proj;

        _renderThread->StopRender();
        _renderer->SetCamera(_viewMatrix, _projMatrix);
        needStartRender = true;
    }

    // check if the data window has changed

    const GfRect2i dataWindow = _GetDataWindow(renderPassState);

    if (_dataWindow != dataWindow) {
        _dataWindow = dataWindow;

        _renderThread->StopRender();
        _renderer->SetDataWindow(dataWindow);

        if (!renderPassState->GetFraming().IsValid()) {
            const GfVec3i dimensions(_dataWindow.GetWidth(), _dataWindow.GetHeight(), 1);

            _colorBuffer.Allocate(
                dimensions,
                HdFormatUNorm8Vec4,
                false // multisampled
            );

            _depthBuffer.Allocate(
                dimensions,
                HdFormatFloat32,
                /*multiSampled=*/false);
        }
        needStartRender = true;
    }

    HdRenderPassAovBindingVector aovBindings = renderPassState->GetAovBindings();
    if (_aovBindings != aovBindings || _renderer->GetAovBindings().empty()) {
        _aovBindings = aovBindings;

        _renderThread->StopRender();

        if (aovBindings.empty()) {
            HdRenderPassAovBinding colorAov;
            colorAov.aovName = HdAovTokens->color;
            colorAov.renderBuffer = &_colorBuffer;
            colorAov.clearValue = VtValue(GfVec4d(0,0,0,1));
            aovBindings.push_back(colorAov);
            HdRenderPassAovBinding depthAov;
            depthAov.aovName = HdAovTokens->depth;
            depthAov.renderBuffer = &_depthBuffer;
            depthAov.clearValue = VtValue(1.0f);
            aovBindings.push_back(depthAov);
        }

        _renderer->SetAovBindings(aovBindings);
        _renderer->Clear();
        needStartRender = true;
    }

    TF_VERIFY(!_aovBindings.empty(), "No aov bindings to render into");

    if (needStartRender) {
        _converged = false;
        _renderer->MarkAovBuffersUnconverged();
        _renderThread->StartRender();
        auto lock = _renderThread->LockFramebuffer();
        // blit pixels from shared to application buffer.
    }
}



PXR_NAMESPACE_CLOSE_SCOPE