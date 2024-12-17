// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <pxr/imaging/hd/mesh.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

class HdStDrawItem;
class HdTemplateRenderParam;

/// \class HdTemplateMesh
///
/// hdTemplate RPrim object
/// - supports tri and quad meshes, as well as subd and curves.
///
class HdTemplateMesh final : public HdMesh {
public:
    HF_MALLOC_TAG_NEW("new HdTemplateMesh");

    ///   \param id scenegraph path
    ///
    HdTemplateMesh(SdfPath const& id);

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    virtual void Sync(
        pxr::HdSceneDelegate* sceneDelegate,
        pxr::HdRenderParam* renderParam,
        pxr::HdDirtyBits* dirtBits,
        const pxr::TfToken& reprToken
    ) override;

    virtual void Finalize(HdRenderParam* renderParam) override;



protected:

    virtual void _InitRepr(TfToken const &reprToken,
            HdDirtyBits *dirtBits
        ) override;

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    // This class does not support copying.
    HdTemplateMesh(const HdTemplateMesh&) = delete;
    HdTemplateMesh& operator=(const HdTemplateMesh&) = delete;
};