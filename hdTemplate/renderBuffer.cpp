//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "renderBuffer.h"
#include "renderParam.h"
#include "pxr/base/gf/half.h"

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderBuffer::HdTemplateRenderBuffer(SdfPath const &id)
    : HdRenderBuffer(id), _width(0), _height(0), _format(HdFormatInvalid), _multiSampled(false), _buffer(), _sampleBuffer(), _sampleCount(), _mappers(0), _converged(false)
{
}

HdTemplateRenderBuffer::~HdTemplateRenderBuffer() = default;

/*virtual*/
void HdTemplateRenderBuffer::Sync(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  HdDirtyBits *dirtyBits)
{
    if (*dirtyBits & DirtyDescription)
    {
        // Template has the background thread write directly into render buffers,
        // so we need to stop the render thread before reallocating them.
        static_cast<HdTemplateRenderParam *>(renderParam)->AcquireSceneForEdit();
    }

    HdRenderBuffer::Sync(sceneDelegate, renderParam, dirtyBits);
}

/*virtual*/
void HdTemplateRenderBuffer::Finalize(HdRenderParam *renderParam)
{
    // Template has the background thread write directly into render buffers,
    // so we need to stop the render thread before removing them.
    static_cast<HdTemplateRenderParam *>(renderParam)->AcquireSceneForEdit();

    HdRenderBuffer::Finalize(renderParam);
}

/*virtual*/
void HdTemplateRenderBuffer::_Deallocate()
{
    // If the buffer is mapped while we're doing this, there's not a great
    // recovery path...
    TF_VERIFY(!IsMapped());

    _width = 0;
    _height = 0;
    _format = HdFormatInvalid;
    _multiSampled = false;
    _buffer.resize(0);
    _sampleBuffer.resize(0);
    _sampleCount.resize(0);

    _mappers.store(0);
    _converged.store(false);
}

/*static*/
size_t
HdTemplateRenderBuffer::_GetBufferSize(GfVec2i const &dims, HdFormat format)
{
    return dims[0] * dims[1] * HdDataSizeOfFormat(format);
}

/*static*/
HdFormat
HdTemplateRenderBuffer::_GetSampleFormat(HdFormat format)
{
    HdFormat component = HdGetComponentFormat(format);
    size_t arity = HdGetComponentCount(format);

    if (component == HdFormatUNorm8 || component == HdFormatSNorm8 ||
        component == HdFormatFloat16 || component == HdFormatFloat32)
    {
        if (arity == 1)
        {
            return HdFormatFloat32;
        }
        else if (arity == 2)
        {
            return HdFormatFloat32Vec2;
        }
        else if (arity == 3)
        {
            return HdFormatFloat32Vec3;
        }
        else if (arity == 4)
        {
            return HdFormatFloat32Vec4;
        }
    }
    else if (component == HdFormatInt32)
    {
        if (arity == 1)
        {
            return HdFormatInt32;
        }
        else if (arity == 2)
        {
            return HdFormatInt32Vec2;
        }
        else if (arity == 3)
        {
            return HdFormatInt32Vec3;
        }
        else if (arity == 4)
        {
            return HdFormatInt32Vec4;
        }
    }
    return HdFormatInvalid;
}

/*virtual*/
bool HdTemplateRenderBuffer::Allocate(GfVec3i const &dimensions,
                                      HdFormat format,
                                      bool multiSampled)
{
    _Deallocate();

    if (dimensions[2] != 1)
    {
        TF_WARN("Render buffer allocated with dims <%d, %d, %d> and"
                " format %s; depth must be 1!",
                dimensions[0], dimensions[1], dimensions[2],
                TfEnum::GetName(format).c_str());
        return false;
    }

    _width = dimensions[0];
    _height = dimensions[1];
    _format = format;
    _buffer.resize(_GetBufferSize(GfVec2i(_width, _height), format));

    _multiSampled = multiSampled;
    if (_multiSampled)
    {
        _sampleBuffer.resize(_GetBufferSize(GfVec2i(_width, _height),
                                            _GetSampleFormat(format)));
        _sampleCount.resize(_width * _height);
    }

    return true;
}

template <typename T>
static void _WriteSample(HdFormat format, uint8_t *dst,
                         size_t valueComponents, T const *value)
{
    HdFormat componentFormat = HdGetComponentFormat(format);
    size_t componentCount = HdGetComponentCount(format);

    for (size_t c = 0; c < componentCount; ++c)
    {
        if (componentFormat == HdFormatInt32)
        {
            ((int32_t *)dst)[c] +=
                (c < valueComponents) ? (int32_t)(value[c]) : 0;
        }
        else
        {
            ((float *)dst)[c] +=
                (c < valueComponents) ? (float)(value[c]) : 0.0f;
        }
    }
}

template <typename T>
static void _WriteOutput(HdFormat format, uint8_t *dst,
                         size_t valueComponents, T const *value)
{
    HdFormat componentFormat = HdGetComponentFormat(format);
    size_t componentCount = HdGetComponentCount(format);

    for (size_t c = 0; c < componentCount; ++c)
    {
        if (componentFormat == HdFormatInt32)
        {
            ((int32_t *)dst)[c] =
                (c < valueComponents) ? (int32_t)(value[c]) : 0;
        }
        else if (componentFormat == HdFormatFloat16)
        {
            ((uint16_t *)dst)[c] =
                (c < valueComponents) ? GfHalf(value[c]).bits() : 0;
        }
        else if (componentFormat == HdFormatFloat32)
        {
            ((float *)dst)[c] =
                (c < valueComponents) ? (float)(value[c]) : 0.0f;
        }
        else if (componentFormat == HdFormatUNorm8)
        {
            ((uint8_t *)dst)[c] =
                (c < valueComponents) ? (uint8_t)(value[c] * 255.0f) : 0.0f;
        }
        else if (componentFormat == HdFormatSNorm8)
        {
            ((int8_t *)dst)[c] =
                (c < valueComponents) ? (int8_t)(value[c] * 127.0f) : 0.0f;
        }
    }
}

void HdTemplateRenderBuffer::Write(
    GfVec3i const &pixel, size_t numComponents, float const *value)
{
    size_t idx = pixel[1] * _width + pixel[0];
    if (_multiSampled)
    {
        size_t formatSize = HdDataSizeOfFormat(_GetSampleFormat(_format));
        uint8_t *dst = &_sampleBuffer[idx * formatSize];
        _WriteSample(_format, dst, numComponents, value);
        _sampleCount[idx]++;
    }
    else
    {
        size_t formatSize = HdDataSizeOfFormat(_format);
        uint8_t *dst = &_buffer[idx * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void HdTemplateRenderBuffer::Write(
    GfVec3i const &pixel, size_t numComponents, int const *value)
{
    size_t idx = pixel[1] * _width + pixel[0];
    if (_multiSampled)
    {
        size_t formatSize = HdDataSizeOfFormat(_GetSampleFormat(_format));
        uint8_t *dst = &_sampleBuffer[idx * formatSize];
        _WriteSample(_format, dst, numComponents, value);
        _sampleCount[idx]++;
    }
    else
    {
        size_t formatSize = HdDataSizeOfFormat(_format);
        uint8_t *dst = &_buffer[idx * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }
}

void HdTemplateRenderBuffer::Clear(size_t numComponents, float const *value)
{
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width * _height; ++i)
    {
        uint8_t *dst = &_buffer[i * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }

    if (_multiSampled)
    {
        std::fill(_sampleCount.begin(), _sampleCount.end(), 0);
        std::fill(_sampleBuffer.begin(), _sampleBuffer.end(), 0);
    }
}

void HdTemplateRenderBuffer::Clear(size_t numComponents, int const *value)
{
    size_t formatSize = HdDataSizeOfFormat(_format);
    for (size_t i = 0; i < _width * _height; ++i)
    {
        uint8_t *dst = &_buffer[i * formatSize];
        _WriteOutput(_format, dst, numComponents, value);
    }

    if (_multiSampled)
    {
        std::fill(_sampleCount.begin(), _sampleCount.end(), 0);
        std::fill(_sampleBuffer.begin(), _sampleBuffer.end(), 0);
    }
}


/*virtual*/
void HdTemplateRenderBuffer::Resolve()
{
    // Resolve the image buffer: find the average value per pixel by
    // dividing the summed value by the number of samples.

    if (!_multiSampled)
    {
        return;
    }

    HdFormat componentFormat = HdGetComponentFormat(_format);
    size_t componentCount = HdGetComponentCount(_format);
    size_t formatSize = HdDataSizeOfFormat(_format);
    size_t sampleSize = HdDataSizeOfFormat(_GetSampleFormat(_format));

    for (unsigned int i = 0; i < _width * _height; ++i)
    {

        int sampleCount = _sampleCount[i];
        // Skip pixels with no samples.
        if (sampleCount == 0)
        {
            continue;
        }

        uint8_t *dst = &_buffer[i * formatSize];
        uint8_t *src = &_sampleBuffer[i * sampleSize];
        for (size_t c = 0; c < componentCount; ++c)
        {
            if (componentFormat == HdFormatInt32)
            {
                ((int32_t *)dst)[c] = ((int32_t *)src)[c] / sampleCount;
            }
            else if (componentFormat == HdFormatFloat16)
            {
                ((uint16_t *)dst)[c] = GfHalf(
                                           ((float *)src)[c] / sampleCount)
                                           .bits();
            }
            else if (componentFormat == HdFormatFloat32)
            {
                ((float *)dst)[c] = ((float *)src)[c] / sampleCount;
            }
            else if (componentFormat == HdFormatUNorm8)
            {
                ((uint8_t *)dst)[c] = (uint8_t)(((float *)src)[c] * 255.0f / sampleCount);
            }
            else if (componentFormat == HdFormatSNorm8)
            {
                ((int8_t *)dst)[c] = (int8_t)(((float *)src)[c] * 127.0f / sampleCount);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE