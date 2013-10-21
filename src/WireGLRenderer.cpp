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

namespace Reyes
{

	WireGLRenderer::WireGLRenderer():
		_shader("wire"),
		_vbo(64)
	{

	}

    void WireGLRenderer::prepare()
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT);

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
        _patch_index[patches_handle] = patch_data;
    }


    void WireGLRenderer::draw_patches(void* patches_handle,
                                      const mat4& matrix,
                                      const Projection& projection,
                                      const vec4& color)
    {
        mat4 proj;
        projection.calc_projection(proj);
        
        _shader.set_uniform("color", color);
        _shader.set_uniform("projection", proj);

        projection.calc_projection_with_aspect_correction(proj);
        
        vector<BezierPatch> stack;
        BezierPatch p0, p1;
        int s = config.bound_n_split_limit();

        const vector<BezierPatch>& patch_list = _patch_index[patches_handle];

        stack.resize(patch_list.size());
        for (size_t i = 0; i < patch_list.size(); ++i) {
            transform_patch(patch_list[i], matrix, stack[i]);
        }
        
        BBox box;
        
        while (!stack.empty()) {

            BezierPatch p = stack.back();
            stack.pop_back();

            calc_bbox(p, box);

            vec2 size;
            bool cull;
            projection.bound(box, size, cull);
            
            if (cull) continue;
            
            if (box.min.z < 0 && size.x < s && size.y < s) {
                draw_patch(p);
            } else {
                pisplit_patch(p, p0, p1, proj);
                stack.push_back(p0);
                stack.push_back(p1);
            }                
        }
        
    }
                                      

    void WireGLRenderer::draw_patch(const BezierPatch& patch)
    {
        vec3 p;

		_vbo.clear();

        int n = 16;

        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(0,0), P(1,0), P(2,0), P(3,0), t, p);
            _vbo.vertex(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(3,0), P(3,1), P(3,2), P(3,3), t, p);
            _vbo.vertex(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(3,3), P(2,3), P(1,3), P(0,3), t, p);
            _vbo.vertex(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(0,3), P(0,2), P(0,1), P(0,0), t, p);
            _vbo.vertex(p.x, p.y, p.z);
        }

        _vbo.send_data();
		
		
		_vbo.draw(GL_LINE_LOOP, _shader);
		
    }
}
