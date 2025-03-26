#include "sceneData.h"
#include <bits/stdc++.h>
#include <random>

PXR_NAMESPACE_OPEN_SCOPE

static float randomFloat()
{
    return ((float)(rand()) / (float)(RAND_MAX)-0.5f) * 2.0f;
}

// Helper function to generate a random vector within a hemisphere
static GfVec3f RandomHemisphereDirection(const GfVec3f &normal)
{
    // Create a random vector in the tangent plane
    GfVec3f randomDirection;

    while (true)
    {
        randomDirection = GfVec3f(randomFloat(), randomFloat(), randomFloat());

        if (randomDirection.GetLength() < 1.0f)
        {
            randomDirection = randomDirection.GetNormalized();
            break;
        }
    }

    // If the normal is pointing upwards (or along a specific axis), make sure the random vector is in the hemisphere
    if (randomDirection * normal < 0.0f)
    {
        randomDirection = -randomDirection; // Reflect the vector to ensure it's in the correct hemisphere
    }

    return randomDirection;
};

SceneData::SceneData(HdRenderIndex *index)
{
    const SdfPathVector rprimIds = index->GetRprimIds();

    for (const SdfPath &rprimId : rprimIds)
    {
        // Retrieve the Rprim object from the render index using the rprimId
        const HdRprim *rprim = index->GetRprim(rprimId);

        // Try casting the Rprim to an HdTemplateMesh object
        if (const HdTemplateMesh *mesh = dynamic_cast<const HdTemplateMesh *>(rprim))
        {
            // If the cast is successful, add the mesh to the collection
            _meshes.push_back(mesh);
        }
    }
}

void SceneData::BuildBVH()
{
    if (_meshes.empty())
        return;

    std::vector<const HdTemplateMesh *> meshes = _meshes;
    _bvhRoot = BuildBVHRecursive(meshes);
}

BVHNode *SceneData::BuildBVHRecursive(std::vector<const HdTemplateMesh *> &meshes)
{
    if (meshes.size() == 1)
    {
        // Create a leaf node with the single mesh
        BVHNode *node = new BVHNode();
        node->bbox = meshes[0]->GetBBox();
        node->mesh = meshes[0];
        return node;
    }

    // Calculate the bounding box of all meshes in this node
    GfRange3d brange;
    for (const HdTemplateMesh *mesh : meshes)
    {
        brange.UnionWith(mesh->GetBBox().GetBox());
    }

    GfBBox3d bbox(brange);

    // Split the meshes along the middle of their bounding boxes
    // For simplicity, let's split along the X-axis. You could split in other ways.
    std::sort(meshes.begin(), meshes.end(), [&](const HdTemplateMesh *a, const HdTemplateMesh *b)
              { return a->GetBBox().GetBox().GetMin()[0] < b->GetBBox().GetBox().GetMin()[0]; });

    size_t mid = meshes.size() / 2;
    std::vector<const HdTemplateMesh *> leftMeshes(meshes.begin(), meshes.begin() + mid);
    std::vector<const HdTemplateMesh *> rightMeshes(meshes.begin() + mid, meshes.end());

    // Create internal nodes
    BVHNode *node = new BVHNode();
    node->bbox = bbox;
    node->left = BuildBVHRecursive(leftMeshes);
    node->right = BuildBVHRecursive(rightMeshes);
    return node;
}

HitData SceneData::Intersect(GfRay ray, int num_bounces)
{
    IntersectData closestIT{
        std::numeric_limits<double>::infinity(),
        GfVec3f(0.0f)};

    const HdTemplateMesh *closestMesh = nullptr;

    // Start traversing the BVH
    closestIT = IntersectBVH(ray, _bvhRoot, closestIT);

    if (closestIT.t < std::numeric_limits<double>::infinity())
    {
        return HitData{
            GetCd(closestIT, ray, num_bounces),
            closestIT.N,
            GfVec3f(ray.GetPoint(closestIT.t)),
            closestIT.t};
    }
    else
    {
        return HitData{};
    }
}


static GfVec3f clamp(GfVec3f a, GfVec3f b) {
    return GfVec3f(std::min(a[0], b[0]),std::min(a[1], b[1]),std::min(a[2], b[2]));
}


GfVec4f SceneData::GetCd(IntersectData it, GfRay ray, int depth)
{
    if (depth == 0)
    {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    GfVec3f light = GfVec3f(-12, -9, -8).GetNormalized();
    float intensity = 2.0f;

    float illum = std::max(it.N * -light, 0.0f) * intensity;

    GfRay shadow_ray(ray.GetPoint(it.t), -light);

    IntersectData shadowIT = IntersectBVH(shadow_ray, _bvhRoot, IntersectData{
        std::numeric_limits<double>::infinity(),
        GfVec3f(0.0f)}
    );

    if (shadowIT.t < std::numeric_limits<double>::infinity())
    {
        illum = 0.0f;
    }


    GfVec3f diffuse = clamp(it.Cd * illum, GfVec3f(1.0f));

    GfVec3f randomDirection = RandomHemisphereDirection(it.N);

    GfRay new_ray(ray.GetPoint(it.t), randomDirection);

    IntersectData closestIT{
        std::numeric_limits<double>::infinity(),
        GfVec3f(0.0f)};

    // Start traversing the BVH
    closestIT = IntersectBVH(new_ray, _bvhRoot, closestIT);

    GfVec3f bounce = GfVec3f(0.0f);

    if (closestIT.t < std::numeric_limits<double>::infinity())
    {
        GfVec4f bounceCd = GetCd(closestIT, new_ray, depth - 1);

        bounce = GfVec3f(bounceCd[0], bounceCd[1], bounceCd[2]);
    }


    GfVec3f Cd = diffuse + it.Cd.GetLength() * bounce;



    return GfVec4f(Cd[0], Cd[1], Cd[2], 1.0f);
}

IntersectData SceneData::IntersectBVH(GfRay ray, BVHNode *node, IntersectData closestIT)
{
    if (!node)
        return closestIT; // If node is null, return the closest intersection found so far

    // Test if the ray intersects the bounding box of the current node
    if (!ray.Intersect(node->bbox))
        return closestIT; // If no intersection with the bounding box, skip this node

    // If it's a leaf node, check intersection with the mesh
    if (node->IsLeaf())
    {
        IntersectData it = node->mesh->Intersect(ray);

        // If a valid intersection is found (t >= 0), and it's closer than the previous closest, update
        if (it.t >= 0.0 && it.t < closestIT.t)
        {
            closestIT = it;
        }
        return closestIT; // Return the closest intersection found so far
    }

    // Otherwise, recursively check left and right children
    closestIT = IntersectBVH(ray, node->left, closestIT);
    closestIT = IntersectBVH(ray, node->right, closestIT);

    return closestIT; // Return the closest intersection after checking both children
}

PXR_NAMESPACE_CLOSE_SCOPE