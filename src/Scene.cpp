#include "common.h"

#include "Scene.h"

#include "Projection.h"
#include "Renderer.h"
#include "Patch.h"

namespace Reyes {
    Scene::Scene (Projection* projection) :
        _projection(projection), 
        _view(), 
        _patches()
    {

    }

    Scene::~Scene()
    {
        delete _projection;
    }
    
    void Scene::set_view(const mat4& view)
    {
        _view = view;
    }

    void Scene::add_patches(const string& filename)
    {
        read_patches(filename.c_str(), _patches);
    }

    const Projection* Scene::get_projection() const
    {
        return _projection;
    }

    const mat4& Scene::get_view() const
    {
        return _view;
    }

    size_t Scene::get_patch_count() const
    {
        return _patches.size();
    }

    const BezierPatch& Scene::get_patch(size_t id) const
    {
        return _patches.at(id);
    }
 
    void Scene::draw(Renderer& renderer) const
    {
        renderer.set_projection(*_projection);

        BezierPatch patch;
        mat4x3 matrix(_view);
        for (size_t i = 0; i < _patches.size(); ++i) {
            
            transform_patch(_patches[i], matrix, patch);

            bound_n_split(patch, *_projection, renderer);

        }
    }
       
}
