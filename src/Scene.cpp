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
#include "PatchDrawer.h"
#include "Patch.h"

#include "Config.h"
#include "Statistics.h"

#include <fcntl.h>
#include <unistd.h>
#include "mscene.capnp.h"
#include "capnp/message.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"

namespace {

    vec3 to_vec3(const ::Vec3::Reader& vec)
    {
        return vec3(vec.getX(), vec.getY(), vec.getZ());
    }

    quat to_quat(const ::Quaternion::Reader& q)
    {
        return quat(q.getR(), q.getI(), q.getJ(), q.getK());
    }

    mat4 transformToMatrix(const ::Transform::Reader& transform)
    {
        vec3 translation = to_vec3(transform.getTranslation());
        quat rotation = to_quat(transform.getRotation());                      
                         
        return glm::translate(translation) * glm::mat4_cast(rotation);
    }

}

Reyes::Scene::Scene (const string& filename) :
    active_cam_id(0)
{
    int fd = open(filename.c_str(), O_RDONLY);

    capnp::PackedFdMessageReader message(fd);

    ::Scene::Reader scene = message.getRoot<::Scene>();

    for (auto c : scene.getCameras()) {
        Camera* camera =
            new Camera{c.getName(),
                       transformToMatrix(c.getTransform()),
                       shared_ptr<Projection>(new PerspectiveProjection(c.getFovy(),
                                                                        c.getNear(),
                                                                        config.window_size()))};
        cameras.push_back(shared_ptr<Camera>(camera));
    }

    for (auto l : scene.getLights()) {
        assert(l.getType() == ::LightSource::Type::DIRECTIONAL);
            
        DirectionalLight* light =
            new DirectionalLight{l.getName(),
                                 vec3(vec4(0,0,1,1) * transformToMatrix(l.getTransform())),
                                 to_vec3(l.getColor()) * l.getIntensity()};

        lights.push_back(shared_ptr<DirectionalLight>(light));
    }

    map<string, shared_ptr<BezierMesh> > meshmap;
    for (auto m : scene.getMeshes()) {
        assert(c.getType() == ::Mesh::Type::BEZIER);

        BezierMesh* mesh = new BezierMesh{m.getName()};

        int i = 0;
        vec3 v;
        BezierPatch patch;
        for (float f : m.getPositions()) {
            v[i%3] = f;

            if (i%3 == 2) {
                patch.P[0][(i/3)%16] = v;
            }

            if (i%(3*16) == (3*16-1)) {
                mesh->patches.push_back(patch);
            }
                
            ++i;
        }

        meshes.push_back(shared_ptr<BezierMesh>(mesh));
        meshmap[mesh->name] = meshes.back();
    }

    for (auto o : scene.getObjects()) {

        shared_ptr<BezierMesh> mesh = meshmap[o.getMeshname()];
        Object* object = new Object{o.getName(),
                                    transformToMatrix(o.getTransform()),
                                    vec4(to_vec3(o.getColor()),1),
                                    mesh};
            
        objects.push_back(shared_ptr<Object>(object));
    }

    close(fd);
}

Reyes::Scene::~Scene()
{
}
     
void Reyes::Scene::draw(PatchDrawer& renderer) const
{
    renderer.prepare();

    for (auto object : objects) {

        // Load patch data on demand
        if (!renderer.are_patches_loaded(object->mesh.get())) {
            renderer.load_patches(object->mesh.get(), object->mesh->patches);
        }

        mat4 matrix(glm::inverse(active_cam().transform) * object->transform);

        renderer.draw_patches(object->mesh.get(), matrix, active_cam().projection.get(), object->color);
            
    }
        
    renderer.finish();
}
       
