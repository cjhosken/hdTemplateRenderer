
#pragma once

#include "mesh.h"
#include <vector>
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/ray.h"

class TemplateScene final 
{
    public:
        /// Renderer constructor.
        TemplateScene();

        TemplateScene(HdRenderIndex* index);

        /// Renderer destructor.
        ~TemplateScene();

        GfVec4f Render(GfRay ray);

        void SortByDepth(GfVec3f origin);

        std::vector<const HdTemplateMesh*> GetMeshes() { return _meshes; }

        TemplateScene(const TemplateScene& other) = default;
        TemplateScene& operator=(const TemplateScene&) = default;

    private:
        HdRenderIndex* _renderIndex;
        std::vector<const HdTemplateMesh*> _meshes;

};