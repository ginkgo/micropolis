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



#version 150

uniform mat4 mvp;
uniform sampler3D patches;

in vec3 vertex;

vec3 eval_patch(vec2 t, int patch_id)
{
    vec2 s = 1 - t;

    mat4 B = outerProduct(vec4(s.x*s.x*s.x, 3*s.x*s.x*t.x, 3*s.x*t.x*t.x, t.x*t.x*t.x),
                          vec4(s.y*s.y*s.y, 3*s.y*s.y*t.y, 3*s.y*t.y*t.y, t.y*t.y*t.y));                          
    
    vec3 sum = vec3(0,0,0);
    for (int u = 0; u < 4; ++u) {
        for (int v = 0; v < 4; ++v) {
            sum += B[v][u] * texelFetch(patches, ivec3(u,v,patch_id), 0).xyz;
        }
    }

    return sum;
}

void main()
{
	gl_Position  = mvp * vec4(eval_patch(vertex.xy, int(vertex.z)),1);
}
