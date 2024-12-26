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
#include "sceneData.h"


PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdTemplateRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). HdTemplate uses this class to pass top-level
/// embree state around.
/// 
class HdTemplateRenderParam : public HdRenderParam {
  public:
    HdTemplateRenderParam(HdRenderThread* renderThread, SceneData *scene, std::atomic<int> *sceneVersion) 
    : _scene(scene), _renderThread(renderThread), _sceneVersion(sceneVersion)
    {

    }

    void SetScene(SceneData *scene) {
      _scene = scene;
    }

    SceneData* AcquireSceneForEdit() {
      _renderThread->StopRender();
      (*_sceneVersion)++;
      return _scene;
    }

private:
    HdRenderThread* _renderThread;
    SceneData* _scene;

    std::atomic<int>* _sceneVersion;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif