#pragma once    

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rect2i.h"

#include "sceneData.h"

#include <random>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdTemplateRenderer final
{
public:
    HdTemplateRenderer();
    ~HdTemplateRenderer();

    void SetScene(SceneData scene);

    void SetDataWindow(const GfRect2i &dataWindow);

    void SetCamera(const GfMatrix4d& viewMatrix, const GfMatrix4d& projMatrix);

    void Render(HdRenderThread *renderThread);

    void SetAovBindings(HdRenderPassAovBindingVector const &aovBindings);

    HdRenderPassAovBindingVector const& GetAovBindings() const {
        return _aovBindings;
    }

    void Clear();

    void MarkAovBuffersUnconverged();

private:

    bool _ValidateAovBindings();

    static GfVec4f _GetClearColor(VtValue const& clearValue);

    // Data window - as in CameraUtilFraming.
    GfRect2i _dataWindow;

    // The bound aovs for this renderer.
    HdRenderPassAovBindingVector _aovBindings;
    // Parsed AOV name tokens.
    HdParsedAovTokenVector _aovNames;

    // Do the aov bindings need to be re-validated?
    bool _aovBindingsNeedValidation;
    // Are the aov bindings valid?
    bool _aovBindingsValid;



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

    SceneData _scene;

};

PXR_NAMESPACE_CLOSE_SCOPE