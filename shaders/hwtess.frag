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

uniform vec4 color;

in vec3 p_eye;
in vec3 n_eye;
in vec3 tex_coord;

out vec4 frag_color;

void main()
{
    // vec3 n = normalize(cross(dFdx(p_eye), dFdy(p_eye)));
	// frag_color = color * max(0, 0.15 + 0.85 * dot(n, vec3(0,0,1)));

    frag_color = color * max(0, dot(n_eye, vec3(0,0,1)));
}
