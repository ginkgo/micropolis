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


#include "HWTessRenderer.h"

#include "Projection.h"
#include "Config.h"
#include "Statistics.h"


namespace Reyes
{

	HWTessRenderer::HWTessRenderer():
		_shader("hwtess"),
		_vbo(4 * config.reyes_patches_per_pass()),
        _patch_count(0)
	{

	}

    void HWTessRenderer::prepare()
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPatchParameteri(GL_PATCH_VERTICES, 4);

		_shader.bind();
    }

    
    void HWTessRenderer::finish()
    {

		_shader.unbind();
        glfwSwapBuffers(glfwGetCurrentContext());
		glfwPollEvents();
    }


    bool HWTessRenderer::are_patches_loaded(void* patches_handle)
    {
        return _patch_index.count(patches_handle) > 0;
    }


    void HWTessRenderer::load_patches(void* patches_handle, const vector<BezierPatch>& patch_data)
    {
        _patch_index[patches_handle].patches = patch_data;
        _patch_index[patches_handle].patch_texture.reset(new GL::TextureBuffer(patch_data.size() * sizeof(BezierPatch), GL_RGB32F));
        _patch_index[patches_handle].patch_texture->load((void*)patch_data.data());

        
    }


    void HWTessRenderer::draw_patches(void* patches_handle,
                                      const mat4& matrix,
                                      const Projection* projection,
                                      const vec4& color)
    {
        mat4 proj;
        projection->calc_projection(proj);

        GL::Tex& patch_tex = *_patch_index[patches_handle].patch_texture;

        patch_tex.bind();
        
        _shader.set_uniform("color", color);
        _shader.set_uniform("mvp", proj * matrix);
        _shader.set_uniform("patches", patch_tex);
        _shader.set_uniform("dicing_rate", (int)config.reyes_patch_size());

        projection->calc_projection_with_aspect_correction(proj);

        mat4 mvp = proj * matrix;
        mat4 mv = matrix;

        statistics.start_bound_n_split();
        vector<PatchRange> stack;
        PatchRange r0, r1;
        int s = config.bound_n_split_limit();

        const vector<BezierPatch>& patch_list = _patch_index[patches_handle].patches;

        stack.resize(patch_list.size());
        for (size_t i = 0; i < patch_list.size(); ++i) {
            stack[i] = PatchRange{Bound(0,0,1,1), 0, i};
        }
        
        BBox box;
        float vlen, hlen;

        while (!stack.empty()) {

            PatchRange r = stack.back();
            stack.pop_back();

            bound_patch_range(r, patch_list[r.patch_id], mv, mvp, box, vlen, hlen);
            
            vec2 size;
            bool cull;
            projection->bound(box, size, cull);
            
            if (cull) continue;
            
            if (box.min.z < 0 && size.x < s && size.y < s) {
                statistics.stop_bound_n_split();
                statistics.inc_patch_count();
                draw_patch(r);
                statistics.start_bound_n_split();
            } else if (r.depth > 20) {
                //cout << "Warning: Split limit reached" << endl;
            } else {
                if (vlen < hlen)
                    vsplit_range(r, r0, r1);
                else
                    hsplit_range(r, r0, r1);
                
                stack.push_back(r0);
                stack.push_back(r1);
            }                
        }
        
        statistics.stop_bound_n_split();

        if (_patch_count > 0) {
            flush();
        }
        
        patch_tex.unbind();
    }
                                      
    
    void HWTessRenderer::draw_patch(const PatchRange& range)
    {
        const Bound& r = range.range;
        const size_t pid = range.patch_id;

        _vbo.vertex(r.min.x, r.min.y, pid);
        _vbo.vertex(r.max.x, r.min.y, pid);
        _vbo.vertex(r.max.x, r.max.y, pid);
        _vbo.vertex(r.min.x, r.max.y, pid);

        _patch_count++;
        
        if (_patch_count >= config.reyes_patches_per_pass()) {
            flush();
        }
    }

    void HWTessRenderer::flush()
    {
        _vbo.send_data();
        _vbo.draw(GL_PATCHES, _shader);
        _vbo.clear();

        _patch_count = 0;        
    }
}
