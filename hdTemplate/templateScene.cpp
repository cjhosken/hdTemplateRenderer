#include "templateScene.h"
#include <iostream>
#include <fstream>
#include <iostream>
#include <string>

TemplateScene::TemplateScene() {}

TemplateScene::TemplateScene(HdRenderIndex *index)
{
    _renderIndex = index;

    const SdfPathVector rprimIds = _renderIndex->GetRprimIds();

    for (const SdfPath &rprimId : rprimIds)
    {
        // Retrieve the Rprim object from the render index using the rprimId
        const HdRprim* rprim = _renderIndex->GetRprim(rprimId);
        
        // Try casting the Rprim to an HdTemplateMesh object
        if (const HdTemplateMesh* mesh = dynamic_cast<const HdTemplateMesh*>(rprim)) {
            // If the cast is successful, add the mesh to the collection
            _meshes.push_back(mesh);
        }
    }
}

void TemplateScene::SortByDepth(GfVec3f origin) {
    std::sort(_meshes.begin(), _meshes.end(), [&](const HdTemplateMesh* a, const HdTemplateMesh* b) {
        float distA = (origin - a->GetPosition()).GetLength();
        float distB = (origin - b->GetPosition()).GetLength();
        return distA > distB;
    });
}

GfVec4f TemplateScene::Render(GfRay ray) {
// Variables to track the closest intersection point
    double closestT = std::numeric_limits<double>::infinity();
    const HdTemplateMesh* closestMesh = nullptr;

    // Iterate over all meshes to find the closest intersection
    for (const HdTemplateMesh* mesh : _meshes) {
        // Check for intersection (assuming Intersect method on HdTemplateMesh)
        double t = mesh->Intersect(ray);  // Assuming this returns the t-value of the intersection

        // If the ray intersects and is closer than the previous closest intersection, update
        if (t >= 0.0 && t < closestT) {
            closestT = t;
            closestMesh = mesh; // Store closest mesh
        }
    }

    // If no intersection found, return default direction (or some indication)
    if (!closestMesh) {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f); // Just return the direction as-is if no intersection
    }

    // Return some representation of the intersection (for example, the hit point, normal, or custom value)
    
    GfVec3d hitPoint = ray.GetPoint(closestT);
    return GfVec4f(1.0f, 1.0f, 1.0f, 1.0f);  // Return hit point as a 4D vector
}

TemplateScene::~TemplateScene()
{
}