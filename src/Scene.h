#ifndef SCENE_H
#define SCENE_H

#include "common.h"

#include "Patch.h"

namespace Reyes
{
    class Projection;

    class Scene
    {
        Projection* _projection;
        mat4 _view;
        vector<BezierPatch> _patches;

        public:

        Scene(Projection* projection);
        ~Scene();

        void set_view(const mat4& view);
        void add_patches(const string& filename);

        const Projection* get_projection() const;
        const mat4& get_view() const;
        size_t get_patch_count() const;
        const BezierPatch& get_patch(size_t id) const;
    };
}

#endif
