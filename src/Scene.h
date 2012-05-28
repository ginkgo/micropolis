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

namespace Reyes
{
    class Projection;
    class PatchDrawer;

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

        void draw(PatchDrawer& renderer) const;
    };
}

#endif
