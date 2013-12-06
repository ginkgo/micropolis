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


#pragma once

#include "Patch.h"

namespace Reyes
{

    class Projection;

    struct Renderer
    {
        virtual ~Renderer() {};

        virtual void prepare() = 0;
        virtual void finish() = 0;

        virtual bool are_patches_loaded(void* patches_handle) = 0;
        virtual void load_patches(void* patches_handle, const vector<BezierPatch>& patch_data) = 0;
        
        virtual void draw_patches(void* patches_handle,
                                  const mat4& matrix,
                                  const Projection* projection,
                                  const vec4& color) = 0;

        virtual void dump_trace() {};
    };

}
