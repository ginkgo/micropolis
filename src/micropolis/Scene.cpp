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
#include "Reyes/Renderer.h"
#include "Patch.h"

#include "Config.h"
#include "ReyesConfig.h"
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

    void from_vec3(const vec3& v, ::Vec3::Builder& vec)
    {
        vec.setX(v.x);
        vec.setY(v.y);
        vec.setZ(v.z);
    }

    
    quat to_quat(const ::Quaternion::Reader& q)
    {
        return quat(q.getR(), q.getI(), q.getJ(), q.getK());
    }

    void from_quat(const quat& q, ::Quaternion::Builder& quat)
    {
        quat.setR(q.w);
        quat.setI(q.x);
        quat.setJ(q.y);
        quat.setK(q.z);
    }
    

    mat4 to_matrix(const ::Transform::Reader& transform)
    {
        vec3 translation = to_vec3(transform.getTranslation());
        quat rotation = to_quat(transform.getRotation());                      
                         
        return glm::translate(translation) * glm::mat4_cast(rotation);
    }

    void from_matrix(const mat4& matrix, ::Transform::Builder& transform)
    {
        vec3 translation(matrix[3]);
        quat rotation = quat(mat3(matrix));

        ::Vec3::Builder trl = transform.initTranslation();
        ::Quaternion::Builder rot = transform.initRotation();

        from_vec3(translation, trl);
        from_quat(rotation, rot);
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
                       to_matrix(c.getTransform()),
                       shared_ptr<Projection>(new Projection(c.getFovy(),
                                                             c.getNear(),
                                                             reyes_config.window_size()))};
        cameras.push_back(shared_ptr<Camera>(camera));
    }

    for (auto l : scene.getLights()) {
        assert(l.getType() == ::LightSource::Type::DIRECTIONAL);
            
        DirectionalLight* light =
            new DirectionalLight{l.getName(),
                                 vec3(vec4(0,0,1,1) * to_matrix(l.getTransform())),
                                 to_vec3(l.getColor()) * l.getIntensity()};

        lights.push_back(shared_ptr<DirectionalLight>(light));
    }

    map<string, shared_ptr<Mesh> > meshmap;
    for (auto m : scene.getMeshes()) {
        
        Reyes::PatchType mesh_type = Reyes::BEZIER;
        switch (m.getType()) {
        case ::Mesh::Type::BEZIER:
            mesh_type = Reyes::BEZIER;
            break;
        case ::Mesh::Type::GREGORY:
            mesh_type = Reyes::GREGORY;
            break;
        }
        
        Mesh* mesh = new Mesh{m.getName(), {}, mesh_type};

        int i = 0;
        vec3 v;
        for (float f : m.getPositions()) {
            v[i%3] = f;

            if (i%3 == 2) {
                mesh->patch_data.push_back(v);
            }
                
            ++i;
        }

        meshes.push_back(shared_ptr<Mesh>(mesh));
        meshmap[mesh->name] = meshes.back();
    }

    for (auto o : scene.getObjects()) {

        shared_ptr<Mesh> mesh = meshmap[o.getMeshname()];
        Object* object = new Object{o.getName(),
                                    to_matrix(o.getTransform()),
                                    vec4(to_vec3(o.getColor()),1),
                                    mesh};
            
        objects.push_back(shared_ptr<Object>(object));
    }

    close(fd);
}

Reyes::Scene::~Scene()
{
}
     
void Reyes::Scene::draw(Renderer& renderer) const
{
    renderer.prepare();

    for (auto object : objects) {

        // Load patch data on demand
        if (!renderer.are_patches_loaded(object->mesh.get())) {
            renderer.load_patches(object->mesh.get(), object->mesh->patch_data, object->mesh->type);
        }

        mat4 matrix(glm::inverse(active_cam().transform) * object->transform);

        renderer.draw_patches(object->mesh.get(), matrix, active_cam().projection.get(), object->color);
            
    }
        
    renderer.finish();
}


void Reyes::Scene::save(const string& base_filename, bool overwrite) const
{
    string filename = base_filename;

    if (!overwrite && file_exists(base_filename)) {
        int i = 0;

        while (file_exists(base_filename + lexical_cast<string>(i))) {
            ++i;
        }

        filename = base_filename + lexical_cast<string>(i);
    }

    cout << "Saving to file '" << filename << "'" << endl;

    ::capnp::MallocMessageBuilder message;
    
    ::Scene::Builder scene = message.initRoot<::Scene>();

    ::capnp::List<::Camera>::Builder _cameras = scene.initCameras(cameras.size());
    for (size_t i = 0; i < cameras.size(); ++i) {
        auto cam = cameras[i];
        _cameras[i].setName(cam->name);

        ::Transform::Builder transform = _cameras[i].initTransform();
        from_matrix(cam->transform, transform);
        
        _cameras[i].setNear(cam->projection->near());
        _cameras[i].setFar(cam->projection->far());
        _cameras[i].setFovy(cam->projection->fovy());        
    }

    ::capnp::List<::LightSource>::Builder _lights = scene.initLights(lights.size());
    for (size_t i = 0; i < lights.size(); ++i) {
        auto light = lights[i];

        _lights[i].setName(light->name);
        ::Transform::Builder transform = _lights[i].initTransform();
        from_matrix(mat4(), transform); // TODO: implement properly

        ::Vec3::Builder color = _lights[i].initColor();
        from_vec3(light->color, color);

        _lights[i].setIntensity(1.0f);
        _lights[i].setType(::LightSource::Type::DIRECTIONAL);
    }

    ::capnp::List<::Object>::Builder _objects = scene.initObjects(objects.size());
    for (size_t i = 0; i < objects.size(); ++i) {
        auto object = objects[i];

        _objects[i].setName(object->name);

        ::Transform::Builder transform = _objects[i].initTransform();
        from_matrix(object->transform, transform);

        ::Vec3::Builder color = _objects[i].initColor();
        from_vec3(vec3(object->color), color);
        
        _objects[i].setMeshname(object->mesh->name);
    }

    ::capnp::List<::Mesh>::Builder _meshes = scene.initMeshes(meshes.size());
    for (size_t i = 0; i < meshes.size(); ++i) {
        auto mesh = meshes[i];

        _meshes[i].setName(mesh->name);
        
        switch(mesh->type) {
        case Reyes::BEZIER:
            _meshes[i].setType(::Mesh::Type::BEZIER);
            break;
        case Reyes::GREGORY:
            _meshes[i].setType(::Mesh::Type::GREGORY);
            break;
        }            

        size_t fcount = mesh->patch_data.size() * 3;
        float* meshdata = (float*)mesh->patch_data.data();
        
        ::capnp::List<float>::Builder _data = _meshes[i].initPositions(fcount);

        for (size_t i = 0; i < fcount; ++i) {
            _data.set(i, meshdata[i]);
        }
    }

    int fd = creat(filename.c_str(), 0664);
    
    writePackedMessageToFd(fd, message);

    close(fd);

}
