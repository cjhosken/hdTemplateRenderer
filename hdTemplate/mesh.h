#ifndef HD_TEMPLATE_MESH_H
#define HD_TEMPLATE_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/gf/ray.h"

PXR_NAMESPACE_OPEN_SCOPE

struct IntersectData {
    double t;
    GfVec3f N;
};

class HdTemplateMesh final : public HdMesh
{
public:
    HF_MALLOC_TAG_NEW("new HdTemplateMesh");

    HdTemplateMesh(SdfPath const &id);

    virtual ~HdTemplateMesh() {};

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam *renderParam,
                      HdDirtyBits *dirtBIts,
                      TfToken const &reprToken) override;

    virtual void Finalize(HdRenderParam *renderParam) override;

    IntersectData Intersect(GfRay ray) const;

protected:
    virtual void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

private:

    void _UpdatePrimvarSources(HdSceneDelegate *sceneDelegate,
                               HdDirtyBits dirtyBits);

    // Populate _primvarSourceMap with primvars that are computed.
    // Return the names of the primvars that were successfully updated.
    TfTokenVector _UpdateComputedPrimvarSources(HdSceneDelegate *sceneDelegate,
                                                HdDirtyBits dirtyBits);

    HdMeshTopology _topology;
    GfMatrix4f _transform;
    VtVec3fArray _points;

    VtVec3iArray _triangulatedIndices;
    VtIntArray _trianglePrimitiveParams;

    struct PrimvarSource
    {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

    HdTemplateMesh(const HdTemplateMesh &) = delete;
    HdTemplateMesh &operator=(const HdTemplateMesh &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif