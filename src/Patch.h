#ifndef PATCH_H
#define PATCH_H

#include "common.h"
#include "Projection.h"

struct BezierPatch
{
    vec3 P[4][4];
};


void read_patches(char* filename, vector<BezierPatch>& patches);

void transform_patch(const BezierPatch& patch, const mat4x3& mat,
                     BezierPatch& out);

void eval_spline(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                 float t, vec3& dst);
void eval_patch(const BezierPatch& patch, float t, float s, vec3& dst);
void eval_patch_n(const BezierPatch& patch,
                  float t, float s, vec3& dst, vec3& normal);


void vsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void hsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void isplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1);
void qsplit_patch(const BezierPatch& patch, 
                  BezierPatch& o0, BezierPatch& o1, 
                  BezierPatch& o2, BezierPatch& o3);

void calc_bbox(const BezierPatch& patch, BBox& box);


void split_n_draw(const BezierPatch& patch, 
                  const Projection& projection,
                  void (*draw_func)(const BezierPatch&));


#endif
