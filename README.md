# HdTemplateRenderer

HdTemplateRenderer is a template folder structure that creates an HdDelegate for USD.

# Info


rendererPlugin.cpp/.h setup the plugin this is how USD finds the plugin. 
renderDelegate.cpp/.h is the actual entry point to your renderer.

Once built, you need to set PXR_PLUGINPATH_NAME=path/to/plug-info-folder
In this case, it would be PXR_PLUGINPATH_NAME=path/to/repository/plugin/usd

## Guides
https://github.com/ospray/hdospray/tree/master
https://github.com/PixarAnimationStudios/OpenUSD/tree/v24.11/pxr/imaging/plugin/hdEmbree
https://github.com/PixarAnimationStudios/OpenUSD/tree/v24.11/pxr/imaging/plugin/hdStorm
https://github.com/dreamworksanimation/hdMoonray
https://github.com/alex-v-dev/hdcycles

