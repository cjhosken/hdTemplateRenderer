//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "rendererPlugin.h"

#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include "renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the embree plugin with the renderer plugin system.
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdTemplateRendererPlugin>();
}

HdRenderDelegate*
HdTemplateRendererPlugin::CreateRenderDelegate()
{
    return new HdTemplateRenderDelegate();
}

HdRenderDelegate*
HdTemplateRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdTemplateRenderDelegate(settingsMap);
}

void
HdTemplateRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}

bool 
HdTemplateRendererPlugin::IsSupported(bool /* gpuEnabled */) const
{
    // Nothing more to check for now, we assume if the plugin loads correctly
    // it is supported.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE