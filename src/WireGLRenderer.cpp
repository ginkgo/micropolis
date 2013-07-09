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

    void WireGLRenderer::set_projection(const Projection& projection)
    {
        mat4 proj;
        projection.calc_projection(proj);
                   
		_shader.bind();
		_shader.set_uniform("projection", proj);
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
