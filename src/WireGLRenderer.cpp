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


#include "WireGLRenderer.h"


#include "Config.h"
#include "Projection.h"
#include "Statistics.h"



Reyes::WireGLRenderer::WireGLRenderer()
    : _shader("wire")
    , _vbo(4 * config.reyes_patches_per_pass())
    , _patch_index(new PatchIndex())
    , _bound_n_split(new BoundNSplit(_patch_index))
{
    _patch_index->enable_load_texture();
}

void Reyes::WireGLRenderer::prepare()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPatchParameteri(GL_PATCH_VERTICES, 4);

}

    
void Reyes::WireGLRenderer::finish()
{
}


bool Reyes::WireGLRenderer::are_patches_loaded(void* patches_handle)
{
    return _patch_index->are_patches_loaded(patches_handle);
}


void Reyes::WireGLRenderer::load_patches(void* patches_handle, const vector<BezierPatch>& patch_data)
{
    _patch_index->load_patches(patches_handle, patch_data);
}


void Reyes::WireGLRenderer::draw_patches(void* patches_handle,
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
    _shader.set_uniform("mvp", proj * matrix);
    _shader.set_uniform("patches", patch_tex);
    _shader.unbind();
    
    _bound_n_split->init(patches_handle, matrix, projection);

    do {

        _bound_n_split->do_bound_n_split(_vbo);

        _shader.bind();
        _shader.set_uniform("flip", GL_FALSE);
        _vbo.draw(GL_PATCHES, _shader);
            
        _shader.set_uniform("flip", GL_TRUE);
        _vbo.draw(GL_PATCHES, _shader);
        _shader.unbind();

    } while (!_bound_n_split->done());
        
    patch_tex.unbind();
}
