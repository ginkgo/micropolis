/******************************************************************************	\
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


#include "RendererGLHWTess.h"

#include "BoundNSplitGLCPU.h"
#include "BoundNSplitGLLocal.h"
#include "BoundNSplitGLMultipass.h"
#include "ReyesConfig.h"
#include "Projection.h"
#include "Statistics.h"


Reyes::RendererGLHWTess::RendererGLHWTess()
    : _shader("hwtess")
    , _vbo(4 * reyes_config.reyes_patches_per_pass())
    , _patch_index(new PatchIndex())
{
    switch(reyes_config.bound_n_split_method()) {
    case ReyesConfig::CPU:
        _bound_n_split.reset(new BoundNSplitGLCPU(_patch_index));
        break;
    default:
        cerr << "Unsupported bound&split method. Falling back to multipass" << endl;
    case ReyesConfig::MULTIPASS:
        _bound_n_split.reset(new BoundNSplitGLMultipass(_patch_index));
        break;
    case ReyesConfig::LOCAL:
        _bound_n_split.reset(new BoundNSplitGLLocal(_patch_index));
        break;
    }
    
    _patch_index->enable_load_texture();
}

void Reyes::RendererGLHWTess::prepare()
{
    //glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPatchParameteri(GL_PATCH_VERTICES, 4);

}

    
void Reyes::RendererGLHWTess::finish()
{
}


bool Reyes::RendererGLHWTess::are_patches_loaded(void* patches_handle)
{
    return _patch_index->are_patches_loaded(patches_handle);
}


void Reyes::RendererGLHWTess::load_patches(void* patches_handle, const vector<BezierPatch>& patch_data)
{
    _patch_index->load_patches(patches_handle, patch_data);
}


void Reyes::RendererGLHWTess::draw_patches(void* patches_handle,
                                         const mat4& matrix,
                                         const Projection* projection,
                                         const vec4& color)
{
    mat4 proj;
    projection->calc_projection(proj);

    GL::Tex& patch_tex = _patch_index->get_patch_texture(patches_handle);

    patch_tex.bind();

    _shader.bind();
    _shader.set_uniform("color", color);
    _shader.set_uniform("mv", matrix);
    _shader.set_uniform("mvp", proj * matrix);
    _shader.set_uniform("patches", patch_tex);
    _shader.set_uniform("dicing_rate", (GLint)reyes_config.reyes_patch_size());
    _shader.unbind();

    _bound_n_split->init(patches_handle, matrix, projection);

    do {
            
        _bound_n_split->do_bound_n_split(_vbo);

        if (!reyes_config.dummy_render()) {
            _shader.bind();
            _vbo.draw(GL_PATCHES, _shader);
            _shader.unbind();
        }
    } while (!_bound_n_split->done());
        
    patch_tex.unbind();
}
