//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_TEMPLATE_RENDER_PARAM_H
#define HD_TEMPLATE_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/usd/usd/prim.h"

#include "templateScene.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdTemplateRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). HdTemplate uses this class to pass top-level
/// embree state around.
/// 
class HdTemplateRenderParam final : public HdRenderParam
{
public:
    HdTemplateRenderParam(HdRenderThread *renderThread, std::atomic<int> *sceneVersion) 
    : _renderThread(renderThread), _sceneVersion(sceneVersion)
    {}

    /// Accessor for the top-level scene.
    void Edit() {
        _renderThread->StopRender();
        (*_sceneVersion)++;
    }

private:

    HdRenderThread *_renderThread;
    std::atomic<int> *_sceneVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif