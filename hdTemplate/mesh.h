// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <pxr/imaging/hd/mesh.h>
#include <pxr/pxr.h>
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/ray.h"

PXR_NAMESPACE_USING_DIRECTIVE

class HdStDrawItem;

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

    double Intersect(GfRay ray) const;

    GfVec3d GetPosition() const {return _position;}

protected:

    virtual void _InitRepr(TfToken const &reprToken,
            HdDirtyBits *dirtBits
        ) override;

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

private:
    GfVec3d _position;
    VtVec3fArray _points;
    HdMeshTopology _topology;

    struct PrimvarSource {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

    // Populate _primvarSourceMap with primvars that are computed.
    // Return the names of the primvars that were successfully updated.
    TfTokenVector _UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
                                                HdDirtyBits dirtyBits);



    // This class does not support copying.
    HdTemplateMesh(const HdTemplateMesh& other) = delete;
    HdTemplateMesh& operator=(const HdTemplateMesh&) = delete;
};