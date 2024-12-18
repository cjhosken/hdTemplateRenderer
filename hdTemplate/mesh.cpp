
// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "mesh.h"
#include "config.h"
#include "context.h"
#include "renderParam.h"
#include "renderPass.h"
#include <pxr/base/gf/ray.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/pxOsd/tokens.h>
#include "pxr/imaging/hd/extComputationUtils.h"

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    HdTemplateTokens,
    (st)
);
// clang-format on

HdTemplateMesh::HdTemplateMesh(SdfPath const &id) : HdMesh(id)
{
}

void HdTemplateMesh::Finalize(HdRenderParam *renderParam)
{
}

double HdTemplateMesh::Intersect(GfRay ray) const
{
    double closestT = std::numeric_limits<double>::infinity(); // Initialize closest intersection as infinite4

    VtIntArray _indices = _topology.GetFaceVertexIndices();

    // Iterate over all triangles in the mesh
    for (size_t i = 0; i < _indices.size(); i += 3)
    {
        // Extract triangle vertices
        GfVec3f p0 = _points[_indices[i]];
        GfVec3f p1 = _points[_indices[i + 1]];
        GfVec3f p2 = _points[_indices[i + 2]];
        
        // Calculate intersection with the triangle using Möller-Trumbore algorithm
        double t;  // Store intersection distance as a double
        GfVec3d barycentricCoords;
        bool frontFacing;
        bool hit = ray.Intersect(p0, p1, p2, &t, &barycentricCoords, &frontFacing);

        // If the intersection is closer than the previous one, update closestT
        if (hit && t < closestT)
        {
            closestT =  t;  // Convert the result back to float
        }
    }

    // Return the closest intersection t-value (or -1.0 if no intersection)
    return closestT == std::numeric_limits<double>::infinity() ? -1.0 : closestT;
}



HdDirtyBits
HdTemplateMesh::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first run
    int mask = HdChangeTracker::Clean | HdChangeTracker::InitRepr | HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyCullStyle | HdChangeTracker::DirtyDoubleSided | HdChangeTracker::DirtyDisplayStyle | HdChangeTracker::DirtySubdivTags | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyNormals | HdChangeTracker::DirtyInstancer;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdTemplateMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void HdTemplateMesh::_InitRepr(TfToken const &reprToken,
                               HdDirtyBits *dirtyBits)
{
    TF_UNUSED(dirtyBits);

    // Create an empty repr.
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    if (it == _reprs.end())
    {
        _reprs.emplace_back(reprToken, HdReprSharedPtr());
    }
}

void HdTemplateMesh::Sync(HdSceneDelegate *sceneDelegate,
                          HdRenderParam *renderParam,
                          HdDirtyBits *dirtyBits,
                          TfToken const &reprToken)
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

    SdfPath const& id = GetId();

    
    TfTokenVector computedPrimvars =
        _UpdateComputedPrimvarSources(sceneDelegate, *dirtyBits);

    
    bool pointsIsComputed =
        std::find(computedPrimvars.begin(), computedPrimvars.end(),
                  HdTokens->points) != computedPrimvars.end();
    
    if (!pointsIsComputed &&
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        VtValue value = sceneDelegate->Get(id, HdTokens->points);
        _points = value.Get<VtVec3fArray>();
    }

    _topology = GetMeshTopology(sceneDelegate);


    GfVec3f sum(0.0f, 0.0f, 0.0f);


    // Assuming you have a sceneDelegate and an id for the object you're querying
    VtValue value = sceneDelegate->Get(id, HdTokens->bbox);

    // Assuming the value contains a GfBBox3f, which is the bounding box type
    GfBBox3d bbox = value.Get<GfBBox3d>();  // Retrieve the bounding box

    // Calculate the center (average position) of the bounding box
    _position = bbox.ComputeCentroid();
}

TfTokenVector
HdTemplateMesh::_UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
                                            HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    
    SdfPath const& id = GetId();

    // Get all the dirty computed primvars
    HdExtComputationPrimvarDescriptorVector dirtyCompPrimvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) {
        HdExtComputationPrimvarDescriptorVector compPrimvars;
        HdInterpolation interp = static_cast<HdInterpolation>(i);
        compPrimvars = sceneDelegate->GetExtComputationPrimvarDescriptors
                                    (GetId(),interp);

        for (auto const& pv: compPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                dirtyCompPrimvars.emplace_back(pv);
            }
        }
    }

    if (dirtyCompPrimvars.empty()) {
        return TfTokenVector();
    }
    
    HdExtComputationUtils::ValueStore valueStore
        = HdExtComputationUtils::GetComputedPrimvarValues(
            dirtyCompPrimvars, sceneDelegate);

    TfTokenVector compPrimvarNames;
    // Update local primvar map and track the ones that were computed
    for (auto const& compPrimvar : dirtyCompPrimvars) {
        auto const it = valueStore.find(compPrimvar.name);
        if (!TF_VERIFY(it != valueStore.end())) {
            continue;
        }
        
        compPrimvarNames.emplace_back(compPrimvar.name);
        if (compPrimvar.name == HdTokens->points) {
            _points = it->second.Get<VtVec3fArray>();
        } else {
            _primvarSourceMap[compPrimvar.name] = {it->second,
                                                compPrimvar.interpolation};
        }
    }

    return compPrimvarNames;
}