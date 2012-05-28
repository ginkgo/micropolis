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


#version 420 compatibility

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 pos[3];
out vec4 color;

void main()
{

    vec3 u = pos[2] - pos[0];
    vec3 w = pos[1] - pos[0];

    vec3 n = -normalize(cross(u,w));

    vec3 l = normalize(vec3(4,3,8));

    vec3 v = -normalize(pos[0]+pos[1]+pos[2]);

    vec4 dc = vec4(0.8, 0.05, 0.01, 1);
    vec4 sc = vec4(1, 1, 1, 1);
        
    vec3 h = normalize(l+v);
        
    float sh = 30.0;

    vec4 c = max(dot(n,l),0.0) * dc + pow(max(dot(n,h), 0.0), sh) * sc;
	
    color = c;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    color = c;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    color = c;
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();


    EndPrimitive();
}
