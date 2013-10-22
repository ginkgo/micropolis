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

#include "Projection.h"
#include "Config.h"

#define P(pi,pj) patch.P[pi][pj]


namespace
{
    using namespace Reyes;

    void vsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1)
    {
        r0 = {r.range, r.depth + 1, r.patch_id};
        r1 = {r.range, r.depth + 1, r.patch_id};
        
        float cy = (r.range.min.y + r.range.max.y) * 0.5f;

        r0.range.min.y = cy;
        r1.range.max.y = cy;
    }
    
    void hsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1)
    {
        r0 = {r.range, r.depth + 1, r.patch_id};
        r1 = {r.range, r.depth + 1, r.patch_id};

        float cx = (r.range.min.x + r.range.max.x) * 0.5f;

        r0.range.min.x = cx;
        r1.range.max.x = cx;
    }


    vec2 project (const vec4& p)
    {
        return vec2(p.x, p.y) / p.w;
    }
    
    void bound_patch_range (const PatchRange& r, const BezierPatch& p, const mat4& mv, const mat4& mvp,
                       BBox& box, float& vlen, float& hlen)
    {
        const size_t RES = 8;
        
        //vec2 pp[RES][RES];
        vec3 ps[RES][RES];
        vec3 pos;
     
        box.clear();
        
        for (int iu = 0; iu < RES; ++iu) {
            for (int iv = 0; iv < RES; ++iv) {
                float u = r.range.min.x + (r.range.max.x - r.range.min.x) * iu * (1.0f / (RES-1));
                float v = r.range.min.y + (r.range.max.y - r.range.min.y) * iv * (1.0f / (RES-1));

                eval_patch(p, u, v, pos);

                vec3 pt = vec3(mv * vec4(pos,1));
                
                box.add_point(pt);

                // pp[iu][iv] = project(mvp * vec4(pos,1));
                ps[iu][iv] = pt;
            }
        }

        vlen = 0;
        hlen = 0;

        for (int i = 0; i < RES; ++i) {
            float h = 0, v = 0;
            for (int j = 0; j < RES-1; ++j) {
                // v += glm::distance(pp[j][i], pp[j+1][i]);
                // h += glm::distance(pp[i][j], pp[i][j+1]);
                v += glm::distance(ps[j][i], ps[j+1][i]);
                h += glm::distance(ps[i][j], ps[i][j+1]);
            }
            vlen = maximum(v, vlen);
            hlen = maximum(h, hlen);
        }
    }

    
};

namespace Reyes
{

	WireGLRenderer::WireGLRenderer():
		_shader("wire"),
		_vbo(4 * config.reyes_patches_per_pass()),
        _patch_count(0)
	{

	}

    void WireGLRenderer::prepare()
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		_shader.bind();
    }

    
    void WireGLRenderer::finish()
    {

		_shader.unbind();
        glfwSwapBuffers(glfwGetCurrentContext());
		glfwPollEvents();
    }


    bool WireGLRenderer::are_patches_loaded(void* patches_handle)
    {
        return _patch_index.count(patches_handle) > 0;
    }


    void WireGLRenderer::load_patches(void* patches_handle, vector<BezierPatch> patch_data)
    {
        _patch_index[patches_handle].patches = patch_data;
        _patch_index[patches_handle].patch_texture.reset
            (new GL::Texture(3, 4, 4, (int)patch_data.size(),
                             GL_RGB, GL_RGB32F,
                             GL_LINEAR, GL_LINEAR, GL_REPEAT,
                             0, (float*)(patch_data.data())));

        
    }


    void WireGLRenderer::draw_patches(void* patches_handle,
                                      const mat4& matrix,
                                      const Projection& projection,
                                      const vec4& color)
    {
        mat4 proj;
        projection.calc_projection(proj);

        GL::Tex& patch_tex = *_patch_index[patches_handle].patch_texture;

        patch_tex.bind();
        
        _shader.set_uniform("color", color);
        _shader.set_uniform("mvp", proj * matrix);
        _shader.set_uniform("patches", patch_tex);

        projection.calc_projection_with_aspect_correction(proj);

        mat4 mvp = proj * matrix;
        mat4 mv = matrix;
        
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

        int patchcnt = 0;
        while (!stack.empty()) {

            PatchRange r = stack.back();
            stack.pop_back();

            bound_patch_range(r, patch_list[r.patch_id], mv, mvp, box, vlen, hlen);
            
            vec2 size;
            bool cull;
            projection.bound(box, size, cull);
            
            if (cull) continue;
            
            if (box.min.z < 0 && size.x < s && size.y < s) {
                draw_patch(r);
                patchcnt++;
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

        if (_patch_count > 0) {
            flush();
        }
        
        patch_tex.unbind();
    }
                                      
    
    void WireGLRenderer::draw_patch(const PatchRange& range)
    {
        const int n = 4;

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

    void WireGLRenderer::flush()
    {
        _vbo.send_data();
        _vbo.draw(GL_QUADS, _shader);
        _vbo.clear();

        _patch_count = 0;        
    }
}
