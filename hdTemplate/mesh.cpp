
// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "mesh.h"
#include "config.h"
#include "context.h"
#include "instancer.h"
#include "renderParam.h"
#include "renderPass.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/pxOsd/tokens.h>


// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    HdTemplateTokens,
    (st)
);
// clang-format on

HdTemplateMesh::HdTemplateMesh(SdfPath const& id) : HdMesh(id)
{
}

void
HdTemplateMesh::Finalize(HdRenderParam *renderParam)
{

}


HdDirtyBits
HdTemplateMesh::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first run
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdTemplateMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdTemplateMesh::_InitRepr(TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(dirtyBits);

    // Create an empty repr.
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    if (it == _reprs.end()) {
        _reprs.emplace_back(reprToken, HdReprSharedPtr());
    }
}

void
HdTemplateMesh::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits,
                   TfToken const   &reprToken)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: A mesh repr can have multiple repr decs; this is done, for example, 
    // when the drawstyle specifies different rasterizing modes between front
    // faces and back faces.
    // With raytracing, this concept makes less sense, but
    // combining semantics of two HdMeshReprDesc is tricky in the general case.
    // For now, HdEmbreeMesh only respects the first desc; this should be fixed.
    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
    const HdMeshReprDesc &desc = descs[0];
}