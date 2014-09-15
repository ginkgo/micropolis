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


#ifndef PATCH_H
#define PATCH_H

#include "common.h"

struct BezierPatch
{
    vec3 P[4][4];
};

namespace Reyes
{
    class Projection;
}

void read_patches(const char* filename, vector<BezierPatch>& patches, bool flip_surface);

void transform_patch(const BezierPatch& patch, const mat4x3& mat,
                     BezierPatch& out);

void transform_patch(const BezierPatch& patch, const mat4& mat,
                     BezierPatch& out);

void eval_spline(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                 float t, vec3& dst);
void eval_patch(const BezierPatch& patch, float t, float s, vec3& dst);
void eval_patch_n(const BezierPatch& patch,
                  float t, float s, vec3& dst, vec3& normal);

void eval_gregory_patch(const vec3* patchdata, float t, float s, vec3& dst);

void vsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void hsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void isplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void pisplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1, const mat4& proj);
void qsplit_patch(const BezierPatch& patch, 
                  BezierPatch& o0, BezierPatch& o1, 
                  BezierPatch& o2, BezierPatch& o3);

void calc_bbox(const BezierPatch& patch, BBox& box);



#endif
