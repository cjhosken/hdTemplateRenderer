#include "renderPass.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdTemplateRenderPass::HdTemplateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection, HdRenderThread *renderThread)
    : HdRenderPass(index, collection)
    , _renderThread(renderThread)
{
}

HdTemplateRenderPass::~HdTemplateRenderPass() {
    _renderThread->StopRender();
}

bool HdTemplateRenderPass::IsConverged() const {
    return false;
}

void HdTemplateRenderPass::_Execute(HdRenderPassStateSharedPtr const &renderPassState,
                                    TfTokenVector const &renderTags)
{
    _renderThread->StartRender();
    auto lock = _renderThread->LockFramebuffer();
    // blit pixels from shared to application buffer.
}



PXR_NAMESPACE_CLOSE_SCOPE