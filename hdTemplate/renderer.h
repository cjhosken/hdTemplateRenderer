//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_TEMPLATE_RENDERER_H
#define HD_TEMPLATE_RENDERER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/usd/usd/prim.h"

#include <random>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdTemplateRenderer
///
/// HdTemplateRenderer implements a renderer on top of Template's raycasting
/// abilities.  This is currently a very simple renderer.  It breaks the
/// framebuffer into tiles for multithreading; sends out jittered camera
/// rays; and implements the following shading:
///  - Colors via the "color" primvar.
///  - Lighting via N dot Camera-ray, simulating a point light at the camera
///    origin.
///  - Ambient occlusion.
///
class HdTemplateRenderer final
{
public:
    /// Renderer constructor.
    HdTemplateRenderer();

    /// Renderer destructor.
    ~HdTemplateRenderer();

    void SetIndex(HdRenderIndex* index);

    /// Set the data window to fill (same meaning as in CameraUtilFraming
    /// with coordinate system also being y-Down).
    void SetDataWindow(const GfRect2i &dataWindow);

    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    void SetAovBindings(HdRenderPassAovBindingVector const &aovBindings);

    HdRenderPassAovBindingVector const& GetAovBindings() const {
        return _aovBindings;
    }

    void Render(HdRenderThread *renderThread);

    void Clear();

    /// Mark the aov buffers as unconverged.
    void MarkAovBuffersUnconverged();
private:
    HdRenderIndex* _renderIndex;

// Validate the internal consistency of aov bindings provided to
    // SetAovBindings. If the aov bindings are invalid, this will issue
    // appropriate warnings. If the function returns false, Render() will fail
    // early.
    //
    // This function thunks itself using _aovBindingsNeedValidation and
    // _aovBindingsValid.
    //   \return True if the aov bindings are valid for rendering.
    bool _ValidateAovBindings();

    // Do the aov bindings need to be re-validated?
    bool _aovBindingsNeedValidation;
    // Are the aov bindings valid?
    bool _aovBindingsValid;

    // Return the clear color to use for the given VtValue.
    static GfVec4f _GetClearColor(VtValue const& clearValue);

    // The bound aovs for this renderer.
    HdRenderPassAovBindingVector _aovBindings;
    // Parsed AOV name tokens.
    HdParsedAovTokenVector _aovNames;

    // Data window - as in CameraUtilFraming.
    GfRect2i _dataWindow;

    // The width of the render buffers.
    unsigned int _width;
    // The height of the render buffers.
    unsigned int _height;

    // View matrix: world space to camera space.
    GfMatrix4d _viewMatrix;
    // Projection matrix: camera space to NDC space.
    GfMatrix4d _projMatrix;
    // The inverse view matrix: camera space to world space.
    GfMatrix4d _inverseViewMatrix;
    // The inverse projection matrix: NDC space to camera space.
    GfMatrix4d _inverseProjMatrix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TEMPLATE_RENDERER_H