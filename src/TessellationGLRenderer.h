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
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef TESSELLATIONGLRENDERER_H
#define TESSELLATIONGLRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

#include "Shader.h"
#include "VBO.h"

namespace Reyes
{

    class TessellationGLRenderer : public PatchDrawer
    {
        GL::Shader _shader;
        GL::VBO _vbo;

    public:

        TessellationGLRenderer();
        ~TessellationGLRenderer();

        virtual void prepare();
        virtual void finish();

        virtual void set_projection(const Projection& projection);
        virtual void draw_patch(const BezierPatch& patch);

        void flush();
    };

}

#endif
