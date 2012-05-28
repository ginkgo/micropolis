/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#include "common.h"

#include "Scene.h"

#include "Projection.h"
#include "Renderer.h"
#include "Patch.h"

#include "Config.h"
#include "Statistics.h"

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
        read_patches(filename.c_str(), _patches, config.flip_surface());
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
 
    void Scene::draw(PatchDrawer& renderer) const
    {
        renderer.prepare();
        renderer.set_projection(*_projection);

        statistics.start_bound_n_split();
        BezierPatch patch;
        mat4x3 matrix(_view);
        for (size_t i = 0; i < _patches.size(); ++i) {
            
            transform_patch(_patches[i], matrix, patch);

            bound_n_split(patch, *_projection, renderer);

        }
        statistics.stop_bound_n_split();

        renderer.finish();
    }
       
}
