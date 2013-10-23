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


#ifndef WIREGLRENDERER_H
#define WIREGLRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

#include "Shader.h"
#include "VBO.h"
#include "Texture.h"

namespace Reyes
{

    struct PatchRange
    {
        Bound range;
        size_t depth;
        size_t patch_id;
    };
        
    class WireGLRenderer : public PatchDrawer
    {
		GL::Shader _shader;
		GL::VBO _vbo;
        size_t _patch_count;

        struct PatchData
        {
            vector<BezierPatch> patches;
            shared_ptr<GL::TextureBuffer> patch_texture;
        };
        
        map<void*, PatchData> _patch_index;

    public:

        WireGLRenderer();
        ~WireGLRenderer() {};

        virtual void prepare();
        virtual void finish();
        
        virtual bool are_patches_loaded(void* patches_handle);
        virtual void load_patches(void* patches_handle, vector<BezierPatch> patch_data);
        
        virtual void draw_patches(void* patches_handle,
                                  const mat4& matrix,
                                  const Projection& projection,
                                  const vec4& color);

    private:

        void draw_patch(const PatchRange& range);
        void flush();
    };
    
}

#endif
