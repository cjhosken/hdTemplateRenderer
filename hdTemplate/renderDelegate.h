#ifndef HD_TEMPLATE_RENDER_DELEGATE_H
#define HD_TEMPLATE_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/base/tf/staticTokens.h"

#include "renderParam.h"
#include "renderPass.h"
#include "sceneData.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdTemplateRenderDelegate final : public HdRenderDelegate
{
public:
    // Default constructor
    HdTemplateRenderDelegate();

    // Constructor with settingsMap argument
    explicit HdTemplateRenderDelegate(const HdRenderSettingsMap &settingsMap);

    // Destructor
    ~HdTemplateRenderDelegate() override;

    HdRenderParam *GetRenderParam() const override;

    const TfTokenVector &GetSupportedRprimTypes() const override;
    /// Return a list of which Sprim types can be created by this class's
    /// CreateSprim.
    const TfTokenVector &GetSupportedSprimTypes() const override;
    /// Return a list of which Bprim types can be created by this class's
    /// CreateBprim.
    const TfTokenVector &GetSupportedBprimTypes() const override;

    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    bool IsPauseSupported() const override;

    /// Pause background rendering threads.
    bool Pause() override;

    /// Resume background rendering threads.
    bool Resume() override;

    // Implementation of pure virtual function(s) from HdRenderDelegate
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                                           HdRprimCollection const &collection) override;

    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                 SdfPath const &id) override;

    void DestroyInstancer(HdInstancer *instancer) override;

    HdRprim *CreateRprim(TfToken const &typeId,
                         SdfPath const &rprimId) override;

    void DestroyRprim(HdRprim *rPrim) override;

    HdSprim *CreateSprim(TfToken const &typeId,
                         SdfPath const &sprimId) override;

    HdSprim *CreateFallbackSprim(TfToken const &typeId) override;

    void DestroySprim(HdSprim *sPrim) override;

    HdBprim *CreateBprim(TfToken const &typeId,
                         SdfPath const &bprimId) override;

    HdBprim *CreateFallbackBprim(TfToken const &typeId) override;

    void DestroyBprim(HdBprim *bPrim) override;

    void CommitResources(HdChangeTracker *tracker) override;

    TfToken GetMaterialBindingPurpose() const override
    {
        return HdTokens->full;
    }

    HdAovDescriptor
    GetDefaultAovDescriptor(TfToken const &name) const override;

    VtDictionary GetRenderStats() const override;

private:
    void _Initialize();

    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    static std::mutex _mutexResourceRegistry;
    static std::atomic_int _counterResourceRegistry;
    static HdResourceRegistrySharedPtr _resourceRegistry;

    HdRenderSettingDescriptorList _settingDescriptors;

    std::shared_ptr<HdTemplateRenderParam> _renderParam;

    // Declare _renderThread only once here
    HdRenderThread _renderThread;

    HdTemplateRenderer _renderer;


    // This class does not support copying.
    HdTemplateRenderDelegate(const HdTemplateRenderDelegate &) = delete;
    HdTemplateRenderDelegate &operator=(const HdTemplateRenderDelegate &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif