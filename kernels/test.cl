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



__kernel void test (write_only image2d_t framebuffer, float t)
{
    int2   coord  = (int2)  (get_global_id(0),   get_global_id(1));
    float2 offset = (float2)(get_global_size(0), get_global_size(1)) * 0.5;
    
    float d = length((float2)(coord.x,coord.y} - offset);
    float r = sin(t * -0.7 + d * 0.13) * 0.5 + 0.5;
    float g = sin(t * -1.3 + d * 0.11) * 0.5 + 0.5;
    float b = sin(t * -1.9 + d * 0.10) * 0.5 + 0.5;
    
    write_imagef(framebuffer, coord, (float4)(r,g,b,1));
}
