#include "renderDelegate.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include <pxr/pxr.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/renderThread.h>

//XXX: Add other Rprim types later
#include "pxr/imaging/hd/camera.h"
//XXX: Add other Sprim types later
#include "pxr/imaging/hd/bprim.h"

#include "renderBuffer.h"

#include "mesh.h"

PXR_NAMESPACE_OPEN_SCOPE

const TfTokenVector HdTemplateRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector HdTemplateRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->extComputation,
};

const TfTokenVector HdTemplateRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer,
};

std::mutex HdTemplateRenderDelegate::_mutexResourceRegistry;
std::atomic_int HdTemplateRenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdTemplateRenderDelegate::_resourceRegistry;

static void _RenderCallback(HdTemplateRenderer* renderer, HdRenderThread *renderThread)
{
    renderer->Clear();
    renderer->Render(renderThread);
}


HdTemplateRenderDelegate::HdTemplateRenderDelegate()
{
    _Initialize();
}

// Constructor with settingsMap argument
HdTemplateRenderDelegate::HdTemplateRenderDelegate(const HdRenderSettingsMap &settingsMap)
{
    _Initialize();
}

void HdTemplateRenderDelegate::_Initialize()
{
    _renderParam = std::make_shared<HdTemplateRenderParam>(
        &_renderThread, nullptr
    );

    _renderer = HdTemplateRenderer();

    // Initialize settingsMap if necessary
    _renderThread.SetRenderCallback(
        std::bind(_RenderCallback, &_renderer, &_renderThread));
    _renderThread.StartThread();

    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);

    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry = std::make_shared<HdResourceRegistry>();
    }
}

// Destructor
HdTemplateRenderDelegate::~HdTemplateRenderDelegate()
{
    {
        std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
        if (_counterResourceRegistry.fetch_sub(1) == 1) {
            _resourceRegistry.reset();
        }
    }
    _renderThread.StopThread();
    _renderParam.reset();
}

HdRenderSettingDescriptorList HdTemplateRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

HdRenderParam* HdTemplateRenderDelegate::GetRenderParam()  const
{
    return _renderParam.get();
}

void HdTemplateRenderDelegate::CommitResources(HdChangeTracker* tracker) 
{
}

TfTokenVector const&
HdTemplateRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdTemplateRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdTemplateRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
HdTemplateRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdAovDescriptor
HdTemplateRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    if (name==HdAovTokens->color) {
        return HdAovDescriptor(HdFormatFloat32Vec4, false, VtValue(GfVec4f(0.0f)));
    } else if (name==HdAovTokens->normal || name == HdAovTokens->Neye) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false, VtValue(GfVec3f(0.0f)));
    } else if (name==HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name==HdAovTokens->cameraDepth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(0.0f));
    } else if (name==HdAovTokens->Peye) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false, VtValue(GfVec3f(0.0f)));
    }
    
    else {
        HdParsedAovToken aovId(name);
        if (aovId.isPrimvar) {
            return HdAovDescriptor(HdFormatFloat32Vec3, false, VtValue(GfVec3f(0.0f)));
        }
    }
    return HdAovDescriptor();
}

VtDictionary 
HdTemplateRenderDelegate::GetRenderStats() const
{
    VtDictionary stats;
    return stats;
}

bool
HdTemplateRenderDelegate::IsPauseSupported() const
{
    return true;
}

bool
HdTemplateRenderDelegate::Pause()
{
    _renderThread.PauseRender();
    return true;
}

bool
HdTemplateRenderDelegate::Resume()
{
    _renderThread.ResumeRender();
    return true;
}

HdRenderPassSharedPtr HdTemplateRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection)
{
    // Implement the required functionality
    return HdRenderPassSharedPtr(new HdTemplateRenderPass(index, collection, &_renderThread, &_renderer)); // Example, create and return a render pass
}

HdInstancer *
HdTemplateRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                        SdfPath const& id)
{
    //return new HdTemplateInstancer(delegate, id);
    return nullptr;
}

void
HdTemplateRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdTemplateRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdTemplateMesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdTemplateRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
HdTemplateRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdSprim *
HdTemplateRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    // For fallback sprims, create objects with an empty scene path.
    // They'll use default values and won't be updated by a scene delegate.
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdTemplateRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
HdTemplateRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdTemplateRenderBuffer(bprimId);
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

HdBprim *
HdTemplateRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdTemplateRenderBuffer(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

void
HdTemplateRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}


PXR_NAMESPACE_CLOSE_SCOPE