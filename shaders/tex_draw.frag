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



#version 330

uniform samplerBuffer framebuffer;
uniform int bsize;
uniform ivec2 gridsize;

void main (void)
{
    ivec2 pxlpos = ivec2(gl_FragCoord.xy);

    ivec2 gridpos = pxlpos / bsize;
    int   grid_id = gridpos.x + gridsize.x * gridpos.y;
    ivec2 loclpos = pxlpos - gridpos * bsize;
    int   locl_id = loclpos.x + bsize * loclpos.y;

    int pos = grid_id * bsize * bsize + locl_id;

    vec4 c = vec4(texelFetch(framebuffer, pos).xyz, 1);

    // Do gamma correction
    c = vec4(pow(c.r, 0.454545), pow(c.g, 0.4545), pow(c.b, 0.4545), c.a);

    gl_FragColor = c;
}
