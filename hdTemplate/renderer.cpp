#include "renderer.h"
#include "sceneData.h"

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderer::HdTemplateRenderer()
    : _width(0), _height(0), _viewMatrix(1.0f), _projMatrix(1.0f), _inverseViewMatrix(1.0f), _inverseProjMatrix(1.0f)
{
}

HdTemplateRenderer::~HdTemplateRenderer() = default;

void HdTemplateRenderer::SetScene(SceneData scene)
{
    _scene = scene;
}

void HdTemplateRenderer::SetDataWindow(const GfRect2i &dataWindow)
{
    _dataWindow = dataWindow;
}

void HdTemplateRenderer::SetCamera(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projMatrix)
{
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _inverseViewMatrix = viewMatrix.GetInverse();
    _inverseProjMatrix = projMatrix.GetInverse();
}

void HdTemplateRenderer::Render(HdRenderThread *renderThread)
{

    for (int y = 0; y < _dataWindow.GetHeight(); ++y)
    {
        for (int x = 0; x < _dataWindow.GetWidth(); ++x)
        {
            if (renderThread->IsStopRequested())
            {
                break;
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE