#include "sceneData.h"

PXR_NAMESPACE_OPEN_SCOPE

SceneData::SceneData(HdRenderIndex* index) {
    const SdfPathVector rprimIds = index->GetRprimIds();

    for (const SdfPath &rprimId : rprimIds)
    {
        // Retrieve the Rprim object from the render index using the rprimId
        const HdRprim* rprim = index->GetRprim(rprimId);
        
        // Try casting the Rprim to an HdTemplateMesh object
        if (const HdTemplateMesh* mesh = dynamic_cast<const HdTemplateMesh*>(rprim)) {
            // If the cast is successful, add the mesh to the collection
            _meshes.push_back(mesh);
        }
    }
}

HitData SceneData::Intersect(GfRay ray) {
// Variables to track the closest intersection point
    IntersectData closestIT{
        std::numeric_limits<double>::infinity(),
        GfVec3f(0.0f)
    };

    const HdTemplateMesh* closestMesh = nullptr;

    // Iterate over all meshes to find the closest intersection

    for (const HdTemplateMesh* mesh : _meshes) {
        // Check for intersection (assuming Intersect method on HdTemplateMesh)
        IntersectData it = mesh->Intersect(ray);  // Assuming this returns the t-value of the intersection

        // If the ray intersects and is closer than the previous closest intersection, update
        if (it.t >= 0.0 && it.t < closestIT.t) {
            closestIT = it;
            closestMesh = mesh; // Store closest mesh
        }
    }

    // If no intersection found, return default direction (or some indication)
    if (!closestMesh) {
        return HitData{nullptr, closestIT}; // Just return the direction as-is if no intersection
    }

    // Return some representation of the intersection (for example, the hit point, normal, or custom value)
    
    return HitData{closestMesh, closestIT};
}

PXR_NAMESPACE_CLOSE_SCOPE