//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_TEMPLATE_RENDERER_PLUGIN_H
#define HD_TEMPLATE_RENDERER_PLUGIN_H

#include <pxr/pxr.h>
#include <pxr/imaging/hd/rendererPlugin.h>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdTemplateRendererPlugin
///
/// A registered child of HdRendererPlugin, this is the class that gets
/// loaded when a hydra application asks to draw with a certain renderer.
/// It supports rendering via creation/destruction of renderer-specific
/// classes. The render delegate is the hydra-facing entrypoint into the
/// renderer; it's responsible for creating specialized implementations of hydra
/// prims (which translate scene data into drawable representations) and hydra
/// renderpasses (which draw the scene to the framebuffer).
///
class HdTemplateRendererPlugin final : public HdRendererPlugin
{
public:
    HdTemplateRendererPlugin() = default;
    virtual ~HdTemplateRendererPlugin() = default;

    /// Construct a new render delegate of type HdTemplateRenderDelegate.
    /// Template render delegates own the embree scene object, so a new render
    /// delegate should be created for each instance of HdRenderIndex.
    ///   \return A new HdTemplateRenderDelegate object.
    HdRenderDelegate *CreateRenderDelegate() override;

    /// Construct a new render delegate of type HdTemplateRenderDelegate.
    /// Template render delegates own the embree scene object, so a new render
    /// delegate should be created for each instance of HdRenderIndex.
    ///   \param settingsMap A list of initialization-time settings for embree.
    ///   \return A new HdTemplateRenderDelegate object.
    HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;

    /// Destroy a render delegate created by this class's CreateRenderDelegate.
    ///   \param renderDelegate The render delegate to delete.
    void DeleteRenderDelegate(
        HdRenderDelegate *renderDelegate) override;

    /// Checks to see if the embree plugin is supported on the running system
    ///
    bool IsSupported(bool gpuEnabled = true) const override;

private:
    // This class does not support copying.
    HdTemplateRendererPlugin(const HdTemplateRendererPlugin&)             = delete;
    HdTemplateRendererPlugin &operator =(const HdTemplateRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TEMPLATE_RENDERER_PLUGIN_H
