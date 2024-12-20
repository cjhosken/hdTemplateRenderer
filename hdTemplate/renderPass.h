//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#pragma once

#include "pxr/pxr.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderThread.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rect2i.h"

#include "renderBuffer.h"
#include "renderer.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdTemplateRenderPass
///
/// HdRenderPass represents a single render iteration, rendering a view of the
/// scene (the HdRprimCollection) for a specific viewer (the camera/viewport
/// parameters in HdRenderPassState) to the current draw target.
///
/// This class does so by raycasting into the embree scene via HdTemplateRenderer.
///

class HdTemplateRenderPass final : public HdRenderPass
{
public:
    HdTemplateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection, HdRenderThread* renderThread, HdTemplateRenderer *renderer);

    ~HdTemplateRenderPass() override;

    bool IsConverged() const override;

protected:
    void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                  TfTokenVector const &renderTags) override;
    
    void _MarkCollectionDirty() override {}

private:
    HdRenderThread* _renderThread;
    HdTemplateRenderer* _renderer;

    GfRect2i _dataWindow;

    GfMatrix4d _viewMatrix;
    GfMatrix4d _projMatrix;

    HdRenderPassAovBindingVector _aovBindings;

    HdTemplateRenderBuffer _colorBuffer;
    HdTemplateRenderBuffer _depthBuffer;

    bool _converged;
};

PXR_NAMESPACE_CLOSE_SCOPE
