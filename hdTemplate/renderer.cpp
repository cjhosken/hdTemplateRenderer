#include "renderer.h"
#include "sceneData.h"
#include "renderBuffer.h"
#include "renderDelegate.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/tf/hash.h"

#include <chrono>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderer::HdTemplateRenderer()
    : _width(0), _height(0), _viewMatrix(1.0f), _projMatrix(1.0f), _inverseViewMatrix(1.0f), _inverseProjMatrix(1.0f), _completedSamples(0)
{
}

static GfVec3f toVec3f(GfVec3d v)
{
    return GfVec3f(
        static_cast<float>(v[0]),
        static_cast<float>(v[1]),
        static_cast<float>(v[2]));
};

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

void HdTemplateRenderer::SetAovBindings(HdRenderPassAovBindingVector const &aovBindings)
{
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    _aovBindingsNeedValidation = true;
}

void HdTemplateRenderer::MarkAovBuffersUnconverged()
{
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        HdTemplateRenderBuffer *rb =
            static_cast<HdTemplateRenderBuffer *>(_aovBindings[i].renderBuffer);
        rb->SetConverged(false);
    }
}

bool HdTemplateRenderer::_ValidateAovBindings()
{
    if (!_aovBindingsNeedValidation)
    {
        return _aovBindingsValid;
    }

    _aovBindingsNeedValidation = false;
    _aovBindingsValid = true;

    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        if (_aovBindings[i].renderBuffer == nullptr)
        {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound", _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->cameraDepth &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->Neye &&
            _aovNames[i].name != HdAovTokens->normal &&

            !_aovNames[i].isPrimvar)
        {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to", _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
             _aovNames[i].name == HdAovTokens->depth) &&
            format != HdFormatFloat32)
        {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        if ((_aovNames[i].name == HdAovTokens->Neye ||
             _aovNames[i].name == HdAovTokens->normal) &&
            format != HdFormatFloat32Vec3)
        {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        if (_aovNames[i].name == HdAovTokens->Peye &&
            format != HdFormatFloat32Vec3)
        {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        if (_aovNames[i].isPrimvar && format != HdFormatFloat32Vec3)
        {
            TF_WARN("Aov 'primvars:%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        if (_aovNames[i].name == HdAovTokens->color)
        {
            switch (format)
            {
            case HdFormatUNorm8Vec4:
            case HdFormatUNorm8Vec3:
            case HdFormatSNorm8Vec4:
            case HdFormatSNorm8Vec3:
            case HdFormatFloat32Vec4:
            case HdFormatFloat32Vec3:
                break;
            default:
                TF_WARN("Aov '%s' has unsupported format '%s'",
                        _aovNames[i].name.GetText(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
                break;
            }
        }

        // make sure the clear value is reasonable for the format of the
        // attached buffer.
        if (!_aovBindings[i].clearValue.IsEmpty())
        {
            HdTupleType clearType =
                HdGetValueTupleType(_aovBindings[i].clearValue);

            // array-valued clear types aren't supported.
            if (clearType.count != 1)
            {
                TF_WARN("Aov '%s' clear value type '%s' is an array",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            if ((_aovNames[i].name == HdAovTokens->cameraDepth || _aovNames[i].name == HdAovTokens->depth) && format != HdFormatFloat32)
            {
                TF_WARN("Aov '%s' has unsupported format '%s'", _aovNames[i].name.GetText(), TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }

            if ((_aovNames[i].name == HdAovTokens->Neye ||
                 _aovNames[i].name == HdAovTokens->normal) &&
                format != HdFormatFloat32Vec3)
            {
                TF_WARN("Aov '%s' has unsupported format '%s'",
                        _aovNames[i].name.GetText(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }

            if (_aovNames[i].name == HdAovTokens->Peye && format != HdFormatFloat32Vec3)
            {
                TF_WARN("Aov '%s' has unsupported format '%s'",
                        _aovNames[i].name.GetText(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }

            // color only supports float/double vec3/4
            if (_aovNames[i].name == HdAovTokens->color &&
                clearType.type != HdTypeFloatVec3 &&
                clearType.type != HdTypeFloatVec4 &&
                clearType.type != HdTypeDoubleVec3 &&
                clearType.type != HdTypeDoubleVec4)
            {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // only clear float formats with float, int with int, float3 with
            // float3.
            if ((format == HdFormatFloat32 && clearType.type != HdTypeFloat) ||
                (format == HdFormatInt32 && clearType.type != HdTypeInt32) ||
                (format == HdFormatFloat32Vec3 &&
                 clearType.type != HdTypeFloatVec3))
            {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible with"
                        " format %s",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }
        }
    }

    return _aovBindingsValid;
}

int
HdTemplateRenderer::GetCompletedSamples() const
{
    return _completedSamples.load();
}

static
bool
_IsContained(const GfRect2i &rect, int width, int height)
{
    return
        rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
        rect.GetMinY() >= 0 && rect.GetMaxY() < height;
}


void HdTemplateRenderer::Render(HdRenderThread *renderThread)
{
    _completedSamples.store(0);

    if (!_ValidateAovBindings()) {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i) {
            HdTemplateRenderBuffer *rb = static_cast<HdTemplateRenderBuffer*>(
                _aovBindings[i].renderBuffer);
            rb->SetConverged(true);
        }
        // XXX:validation
        TF_WARN("Could not validate Aovs. Render will not complete");
        return;
    }

    _width  = 0;
    _height = 0;

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        //
        // XXX
        //
        // A scene delegate might specify the path to a
        // render buffer instead of a pointer to the
        // render buffer.
        //
        static_cast<HdTemplateRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();

        if (i == 0) {
            _width  = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        } else {
            if ( _width  != _aovBindings[i].renderBuffer->GetWidth() ||
                 _height != _aovBindings[i].renderBuffer->GetHeight()) {
                TF_CODING_ERROR(
                    "Render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0) {
        if (!_IsContained(_dataWindow, _width, _height)) {
            TF_CODING_ERROR(
                "dataWindow is larger than render buffer");
        
        }
    }

    for (int i = 0; i < _numSamples; ++i)
    {
        while (renderThread->IsPauseRequested())
        {
            if (renderThread->IsStopRequested())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // Cancellation point.
        if (renderThread->IsStopRequested())
        {
            break;
        }

        const unsigned int numTilesX = (_dataWindow.GetWidth() + _tileSize - 1) / _tileSize;
        const unsigned int numTilesY = (_dataWindow.GetHeight() + _tileSize - 1) / _tileSize;

        WorkParallelForN(numTilesX * numTilesY, std::bind(&HdTemplateRenderer::_RenderTiles, this, renderThread, i, std::placeholders::_1, std::placeholders::_2));

        if (i == 0)
        {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i)
            {
                HdTemplateRenderBuffer *rb = static_cast<HdTemplateRenderBuffer *>(
                    _aovBindings[i].renderBuffer);
                if (rb->IsMultiSampled())
                {
                    moreWork = true;
                }
            }
            if (!moreWork)
            {
                _completedSamples.store(i + 1);
                break;
            }
        }

        // Track the number of completed samples for external consumption.
        _completedSamples.store(i + 1);

        // Cancellation point.
        if (renderThread->IsStopRequested())
        {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap all buffers.
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        HdTemplateRenderBuffer *rb = static_cast<HdTemplateRenderBuffer *>(
            _aovBindings[i].renderBuffer);
        rb->Unmap();
        rb->SetConverged(true);
    }
}

void HdTemplateRenderer::_RenderTiles(HdRenderThread *renderThread, int sampleNum, size_t tileStart, size_t tileEnd)
{
    const unsigned int minX = _dataWindow.GetMinX();
    unsigned int minY = _dataWindow.GetMinY();
    const unsigned int maxX = _dataWindow.GetMaxX() + 1;
    unsigned int maxY = _dataWindow.GetMaxY() + 1;

    const unsigned int numTilesX = (_dataWindow.GetWidth() + _tileSize - 1) / _tileSize;

    GfVec3f origin = GfVec3f(_inverseViewMatrix.Transform(GfVec3f(0.0f)));


    std::default_random_engine random(0);

    // Create a uniform distribution for jitter calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    auto uniform_float = [&random, &uniform_dist]()
    { return uniform_dist(random); };

    for (unsigned int tile = tileStart; tile < tileEnd; ++tile)
    {
        if (renderThread && renderThread->IsStopRequested())
        {
            break;
        }

        const unsigned int tileY = tile / numTilesX;
        const unsigned int tileX = tile - tileY * numTilesX;

        const unsigned int x0 = tileX * _tileSize + minX;
        const unsigned int y0 = tileY * _tileSize + minY;

        const unsigned int x1 = std::min(x0 + _tileSize, maxX);
        const unsigned int y1 = std::min(y0 + _tileSize, maxY);

        const float w(_dataWindow.GetWidth());
        const float h(_dataWindow.GetHeight());

        for (unsigned int y = y0; y < y1; ++y)
        {
            for (unsigned int x = x0; x < x1; ++x)
            {
                if (renderThread && renderThread->IsStopRequested()) {
                    return; // Exit the function early if the render thread is stopped
                }

                GfVec4f Cd(0.0f, 0.0f, 0.0f, 1.0f);
                GfVec3f N(0.0f);
                GfVec3f P(0.0f);
                float z = 0;

                GfVec2f jitter(uniform_float(), uniform_float());
                // GfVec2f jitter(0.0f);

                const GfVec3f ndc(
                    2 * ((x + jitter[0] - _dataWindow.GetMinX()) / w) - 1,
                    2 * ((y + jitter[1] - _dataWindow.GetMinY()) / h) - 1,
                    -1);

                const GfVec3f nearPlaneTrace(_inverseProjMatrix.Transform(ndc));

                GfVec3f dir = GfVec3f(_inverseViewMatrix.TransformDir(nearPlaneTrace)).GetNormalized();

                GfRay ray = GfRay(GfVec3d(origin), GfVec3d(dir));

                HitData hit = _scene.Intersect(ray, _numBounces);

                Cd += hit.Cd;
                N += hit.N;
                P += hit.P;
                z += hit.t;

                for (size_t i = 0; i < _aovBindings.size(); ++i)
                {
                    HdTemplateRenderBuffer *renderBuffer = static_cast<HdTemplateRenderBuffer *>(_aovBindings[i].renderBuffer);

                    if (renderBuffer->IsConverged())
                    {
                        continue;
                    }
                    if (_aovNames[i].name == HdAovTokens->color)
                    {
                        renderBuffer->Write(GfVec3i(x, y, 1), 4, Cd.data());
                    }
                    else if ((_aovNames[i].name == HdAovTokens->cameraDepth || _aovNames[i].name == HdAovTokens->depth) && renderBuffer->GetFormat() == HdFormatFloat32)
                    {
                        renderBuffer->Write(GfVec3i(x, y, 1), 1, &z);
                    }
                    else if (_aovNames[i].name == HdAovTokens->Peye && renderBuffer->GetFormat() == HdFormatFloat32Vec3)
                    {
                        renderBuffer->Write(GfVec3i(x, y, 1), 3, P.data());
                    }
                    else if ((_aovNames[i].name == HdAovTokens->Neye ||
                              _aovNames[i].name == HdAovTokens->normal) &&
                             renderBuffer->GetFormat() == HdFormatFloat32Vec3)
                    {
                        renderBuffer->Write(GfVec3i(x, y, 1), 3, N.data());
                    }
                    else if (_aovNames[i].isPrimvar &&
                             renderBuffer->GetFormat() == HdFormatFloat32Vec3)
                    {
                        GfVec3f value;
                        renderBuffer->Write(GfVec3i(x, y, 1), 3, value.data());
                    }
                }
            }
        }
    }
}

/* static */
GfVec4f
HdTemplateRenderer::_GetClearColor(VtValue const &clearValue)
{
    HdTupleType type = HdGetValueTupleType(clearValue);
    if (type.count != 1)
    {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch (type.type)
    {
    case HdTypeFloatVec3:
    {
        GfVec3f f =
            *(static_cast<const GfVec3f *>(HdGetValueData(clearValue)));
        return GfVec4f(f[0], f[1], f[2], 1.0f);
    }
    case HdTypeFloatVec4:
    {
        GfVec4f f =
            *(static_cast<const GfVec4f *>(HdGetValueData(clearValue)));
        return f;
    }
    case HdTypeDoubleVec3:
    {
        GfVec3d f =
            *(static_cast<const GfVec3d *>(HdGetValueData(clearValue)));
        return GfVec4f(f[0], f[1], f[2], 1.0f);
    }
    case HdTypeDoubleVec4:
    {
        GfVec4d f =
            *(static_cast<const GfVec4d *>(HdGetValueData(clearValue)));
        return GfVec4f(f);
    }
    default:
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void HdTemplateRenderer::Clear()
{
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        if (_aovBindings[i].clearValue.IsEmpty())
        {
            continue;
        }

        HdTemplateRenderBuffer *rb = static_cast<HdTemplateRenderBuffer *>(_aovBindings[i].renderBuffer);

        rb->Map();
        if (_aovNames[i].name == HdAovTokens->color)
        {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            rb->Clear(4, clearColor.data());
        }
        else if (rb->GetFormat() == HdFormatInt32)
        {
            int32_t clearValue = _aovBindings[i].clearValue.Get<int32_t>();
            rb->Clear(1, &clearValue);
        }
        else if (rb->GetFormat() == HdFormatFloat32)
        {
            float clearValue = _aovBindings[i].clearValue.Get<float>();
            rb->Clear(1, &clearValue);
        }
        else if (rb->GetFormat() == HdFormatFloat32Vec3)
        {
            GfVec3f clearValue = _aovBindings[i].clearValue.Get<GfVec3f>();
            rb->Clear(3, clearValue.data());
        } // else, _ValidateAovBindings would have already warned.

        rb->Unmap();
        rb->SetConverged(false);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE