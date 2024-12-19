#pragma once

#include "mesh.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"

#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

struct HitData {
    const HdTemplateMesh* mesh;
    IntersectData it;
};

class SceneData final {
    public:
        SceneData() {}

        SceneData(HdRenderIndex *index);

        HitData Intersect(GfRay ray);

        void SortByDepth(GfVec3f origin);

    private:
        std::vector<const HdTemplateMesh*> _meshes;
};

PXR_NAMESPACE_CLOSE_SCOPE