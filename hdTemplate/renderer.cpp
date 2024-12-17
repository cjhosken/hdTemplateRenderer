#include "renderer.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/tf/hash.h"

#include "renderBuffer.h"
#include "config.h"
#include "mesh.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

HdTemplateRenderer::HdTemplateRenderer()
    : _aovBindings(), _aovNames(), _aovBindingsNeedValidation(false), _aovBindingsValid(false), _width(0), _height(0), _viewMatrix(1.0f) // == identity
      ,
      _projMatrix(1.0f) // == identity
      ,
      _inverseViewMatrix(1.0f) // == identity
      ,
      _inverseProjMatrix(1.0f) // == identity
{
}

HdTemplateRenderer::~HdTemplateRenderer() = default;

void HdTemplateRenderer::SetDataWindow(const GfRect2i &dataWindow)
{
    _dataWindow = dataWindow;

    // Here for clients that do not use camera framing but the
    // viewport.
    //
    // Re-validate the attachments, since attachment viewport and
    // render viewport need to match.
    _aovBindingsNeedValidation = true;
}

void HdTemplateRenderer::SetCamera(const GfMatrix4d &viewMatrix,
                                   const GfMatrix4d &projMatrix)
{
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _inverseViewMatrix = viewMatrix.GetInverse();
    _inverseProjMatrix = projMatrix.GetInverse();
}

void HdTemplateRenderer::SetAovBindings(
    HdRenderPassAovBindingVector const &aovBindings)
{
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    // Re-validate the attachments.
    _aovBindingsNeedValidation = true;
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

        // By the time the attachment gets here, there should be a bound
        // output buffer.
        if (_aovBindings[i].renderBuffer == nullptr)
        {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound",
                    _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->depth &&
            !_aovNames[i].isPrimvar)
        {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                    _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->depth) && format != HdFormatFloat32)
        {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // color is only supported for vec3/vec4 attachments of float,
        // unorm, or snorm.
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
    if (!_ValidateAovBindings())
    {
        return;
    }

    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        if (_aovBindings[i].clearValue.IsEmpty())
        {
            continue;
        }

        HdTemplateRenderBuffer *rb =
            static_cast<HdTemplateRenderBuffer *>(_aovBindings[i].renderBuffer);

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
        }

        rb->Unmap();
        rb->SetConverged(false);
    }
}

static bool
_IsContained(const GfRect2i &rect, int width, int height)
{
    return rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
           rect.GetMinY() >= 0 && rect.GetMaxY() < height;
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

void HdTemplateRenderer::Render(HdRenderThread *renderThread)
{
    if (!_ValidateAovBindings())
    {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i)
        {
            HdTemplateRenderBuffer *rb = static_cast<HdTemplateRenderBuffer *>(
                _aovBindings[i].renderBuffer);
            rb->SetConverged(true);
        }
        // XXX:validation
        TF_WARN("Could not validate Aovs. Render will not complete");
        return;
    }

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i)
    {
        //
        // XXX
        //
        // A scene delegate might specify the path to a
        // render buffer instead of a pointer to the
        // render buffer.
        //
        static_cast<HdTemplateRenderBuffer *>(
            _aovBindings[i].renderBuffer)
            ->Map();

        if (i == 0)
        {
            _width = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        }
        else
        {
            if (_width != _aovBindings[i].renderBuffer->GetWidth() ||
                _height != _aovBindings[i].renderBuffer->GetHeight())
            {
                TF_CODING_ERROR(
                    "Template render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0)
    {
        if (!_IsContained(_dataWindow, _width, _height))
        {
            TF_CODING_ERROR(
                "dataWindow is larger than render buffer");
        }
    }

    _width = 0;
    _height = 0;

    for (int i = 0; i < 10; ++i)
    {
        // Pause point.
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

        for (unsigned int y = 0; y < _dataWindow.GetMaxY(); ++y)
        {
            for (unsigned int x = 0; x < _dataWindow.GetMaxX(); ++x)
            {
                for (size_t i = 0; i < _aovBindings.size(); ++i)
                {
                    HdTemplateRenderBuffer *renderBuffer =
                        static_cast<HdTemplateRenderBuffer *>(_aovBindings[i].renderBuffer);

                    if (_aovNames[i].name == HdAovTokens->color)
                    {
                        GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);

                        float r = x / static_cast<float>(_dataWindow.GetMaxX());
                        float g = y / static_cast<float>(_dataWindow.GetMaxY());
                        float b = 0.5f;

                        GfVec4f sample(r, g, b, 1.0f);
                        renderBuffer->Write(GfVec3i(x, y, 1), 4, sample.data());
                    }
                    else if (_aovNames[i].name == HdAovTokens->depth)
                    {
                        float depth = 0.5f;

                        renderBuffer->Write(GfVec3i(x, y, 1), 1, &depth);
                    }
                }
            }
        }

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