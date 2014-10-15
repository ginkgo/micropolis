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


#ifndef SCENE_H
#define SCENE_H

#include "common.h"

#include "Patch.h"
#include "Reyes/PatchType.h"

namespace Reyes
{
    class Projection;
    class Renderer;

    
    struct DirectionalLight
    {
        string name;
        vec3 dir;
        vec3 color;
    };


    struct Mesh
    {
        string name;
        vector<vec3> patch_data;
        Reyes::PatchType type;
    };


    struct Object
    {
        string name;
        mat4 transform;
        vec4 color;
        shared_ptr<Mesh> mesh;
    };


    struct Camera
    {
        string name;
        mat4 transform;
        shared_ptr<Projection> projection;
    };
    
    
    class Scene
    {
        shared_vector<Camera> cameras;
        shared_vector<DirectionalLight> lights;
        shared_vector<Object> objects;
        shared_vector<Mesh> meshes;

        size_t active_cam_id;
        
        public:

        Scene(const string& filename);
        ~Scene();

        const Camera& active_cam() const { return *cameras[active_cam_id]; }
        Camera& active_cam() { return *cameras[active_cam_id]; }

        void draw(Renderer& renderer) const;

        void save(const string& filename, bool overwrite=false) const;

        size_t total_patch_count() const;
        
    };
}

#endif
