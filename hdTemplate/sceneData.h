#pragma once

#include "mesh.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"

#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

struct HitData {
    GfVec4f Cd;
    GfVec3f N;
    GfVec3f P;
    float t;
};

struct BVHNode {
    GfBBox3d bbox;          // Bounding box of the node
    const HdTemplateMesh* mesh;  // Mesh in this node (if a leaf)
    BVHNode* left = nullptr;    // Left child node
    BVHNode* right = nullptr;   // Right child node

    // Check if the node is a leaf (contains a single mesh)
    bool IsLeaf() const {
        return mesh != nullptr;
    }
};


class SceneData final {
    public:
        SceneData() {}

        void BuildBVH();

        SceneData(HdRenderIndex *index);

        HitData Intersect(GfRay ray, int num_bounces);

        void SortByDepth(GfVec3f origin);

    private:
        BVHNode* BuildBVHRecursive(std::vector<const HdTemplateMesh*>& meshes);

        IntersectData IntersectBVH(GfRay ray, BVHNode* node, IntersectData closestIT);

        GfVec4f GetCd(IntersectData it, GfRay ray, int depth);

        BVHNode* _bvhRoot = nullptr; // Root of the BVH

        std::vector<const HdTemplateMesh*> _meshes;
};

PXR_NAMESPACE_CLOSE_SCOPE