#ifndef PATCH_H
#define PATCH_H

#include "common.h"

struct BezierPatch
{
    vec3 P[4][4];
    vec2 Pproj[4][4];
};


void read_patches(char* filename, vector<BezierPatch>& patches);


void project_node(const vec3& P, const mat4& mat, vec2& Pproj);
void project_patch(BezierPatch& patch, const mat4& mat);

void eval_spline(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                 float t, vec3& dst);
void eval_patch(const BezierPatch& patch, float t, float s, vec3& dst);
void eval_patch_n(const BezierPatch& patch,
                  float t, float s, vec3& dst, vec3& normal);


void vsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1,
                  const mat4& mat);
void hsplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1,
                  const mat4& mat);
void isplit_patch(const BezierPatch& patch, BezierPatch& o0, BezierPatch& o1,
                  const mat4& mat);
void qsplit_patch(const BezierPatch& patch, 
                  BezierPatch& o0, 
                  BezierPatch& o1, 
                  BezierPatch& o2, 
                  BezierPatch& o3,
                  const mat4& mat);

void calc_bbox(const BezierPatch& patch, Box& box);


void split_n_draw(int i, const BezierPatch& patch, const mat4& mat,
                  void (*draw_func)(const BezierPatch&));


#endif
