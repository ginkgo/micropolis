#include "Patch.h"

using Reyes::Projection;

void transform_patch(const BezierPatch& patch, 
                     const mat4x3& mat, 
                     BezierPatch& out)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
             out.P[i][j] = mat * vec4(patch.P[i][j],1.0f);
        }
    }
}

void transform_patch(const BezierPatch& patch, 
                     const mat4& mat, 
                     BezierPatch& out)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
             vec4 pp = mat * vec4(patch.P[i][j],1.0f);

             out.P[i][j] = vec3(pp.x,pp.y,pp.z) / pp.w;
        }
    }
}

void read_patches(const char* filename, vector<BezierPatch>& patches, bool flip_surface)
{
    vector<int> indices;
    vector<vec3> points;
    FILE* file = fopen(filename, "r");    

    if (!file) {
        cerr << "Can't open " << filename << endl;
        exit(1);
    }

    int np, nv;
    int r = fscanf(file, "%i\n", &np);
    indices.resize(np * 16);
    for (int i = 0; i < np; ++i) {
        int *p = &(indices[i*16]);
        if (!flip_surface) {
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 0, p+ 1, p+ 2, p+ 3);
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 4, p+ 5, p+ 6, p+ 7);
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 8, p+ 9, p+10, p+11);
            r = fscanf(file, "%i, %i, %i, %i\n", p+12, p+13, p+14, p+15);
        } else {
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 0, p+ 4, p+ 8, p+12);
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 1, p+ 5, p+ 9, p+13);
            r = fscanf(file, "%i, %i, %i, %i,",  p+ 2, p+ 6, p+10, p+14);
            r = fscanf(file, "%i, %i, %i, %i\n", p+ 3, p+ 7, p+11, p+15);
        }
    }

    BBox bbox;

    r= fscanf(file, "%i\n", &nv);
    points.resize(nv);
    for (int i = 0; i < nv; ++i) {
        float x,y,z;
        r= fscanf(file, "%f, %f, %f\n", &x, &y, &z);
        points[i] = vec3(x,y,z);
        bbox.add_point(vec3(x,y,z));
    }

    vec3 center = bbox.center();

    for (int i = 0; i < nv; ++i) {
        points[i] -= center;
    }

    patches.resize(np);

    int k = 0;
    for (int i = 0; i < np; ++i) {
        vec3* ps = patches[i].P[0];

        for (int j = 0; j < 16; ++j) {
            ps[j] = points[indices[k]-1];
            ++k;
        }
    }
}


void eval_spline(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                 float t, vec3& dst)
{
    float s = 1-t;
    dst = s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;

}

void eval_patch(const BezierPatch& patch,
                float t, float s, vec3& dst)
{
    vec3 p[4];

    for (int i = 0; i < 4; ++i) {
        eval_spline(patch.P[i][0],
                    patch.P[i][1],
                    patch.P[i][2],
                    patch.P[i][3],
                    t, p[i]);
    }

    eval_spline(p[0],p[1],p[2],p[3], s, dst);
}


void eval_patch_n(const BezierPatch& patch,
                  float t, float s, vec3& dst, vec3& normal)
{
    vec3 q[4], dq[4];

    float si = 1-s;
    float ti = 1-t;
    
    for (int i = 0; i < 4; ++i) {
        vec3 p10 = ti * patch.P[i][0] + t * patch.P[i][1];
        vec3 p11 = ti * patch.P[i][1] + t * patch.P[i][2];
        vec3 p12 = ti * patch.P[i][2] + t * patch.P[i][3];
        vec3 p20 = ti * p10 + t * p11;
        vec3 p21 = ti * p11 + t * p12;
        vec3 p30 = ti * p20 + t * p21;

        q[i] = p30;
        if (t < 0.5f)
            dq[i] = p21-p30;
        else 
            dq[i] = p30-p20;
    }

    vec3 q10 = si * q[0] + s * q[1];
    vec3 q11 = si * q[1] + s * q[2];
    vec3 q12 = si * q[2] + s * q[3];
    vec3 q20 = si * q10  + s * q11;
    vec3 q21 = si * q11  + s * q12;
    vec3 q30 = si * q20  + s * q21;

    vec3 dq10 = si * dq[0] + s * dq[1];
    vec3 dq11 = si * dq[1] + s * dq[2];
    vec3 dq12 = si * dq[2] + s * dq[3];
    vec3 dq20 = si * dq10  + s * dq11;
    vec3 dq21 = si * dq11  + s * dq12;
    vec3 dq30 = si * dq20  + s * dq21;

    dst = q30;

    vec3 tng,btng;
    
    tng = dq30;
    
    if (s < 0.5f)
        btng = q21-q30;
    else 
        btng = q30-q20;

    normal = glm::normalize(glm::cross(btng, tng)) * 0.5f + vec3(0.5f);
    
}


void vsplit_patch(const BezierPatch& patch,
                  BezierPatch& o0, BezierPatch& o1)
{
    for (int i = 0; i < 4; ++i) {
        vec3 p10 = 0.5f * patch.P[0][i] + 0.5f * patch.P[1][i];
        vec3 p11 = 0.5f * patch.P[1][i] + 0.5f * patch.P[2][i];
        vec3 p12 = 0.5f * patch.P[2][i] + 0.5f * patch.P[3][i];
        vec3 p20 = 0.5f * p10 + 0.5f * p11;
        vec3 p21 = 0.5f * p11 + 0.5f * p12;
        vec3 p30 = 0.5f * p20 + 0.5f * p21;

        o0.P[0][i] = patch.P[0][i];
        o0.P[1][i] = p10;
        o0.P[2][i] = p20;
        o0.P[3][i] = p30;    

        o1.P[0][i] = p30;
        o1.P[1][i] = p21;
        o1.P[2][i] = p12;
        o1.P[3][i] = patch.P[3][i];
    }
}

void hsplit_patch(const BezierPatch& patch,
                  BezierPatch& o0, BezierPatch& o1)
{
    for (int i = 0; i < 4; ++i) {
        vec3 p10 = 0.5f * patch.P[i][0] + 0.5f * patch.P[i][1];
        vec3 p11 = 0.5f * patch.P[i][1] + 0.5f * patch.P[i][2];
        vec3 p12 = 0.5f * patch.P[i][2] + 0.5f * patch.P[i][3];
        vec3 p20 = 0.5f * p10 + 0.5f * p11;
        vec3 p21 = 0.5f * p11 + 0.5f * p12;
        vec3 p30 = 0.5f * p20 + 0.5f * p21;

        o0.P[i][0] = patch.P[i][0];
        o0.P[i][1] = p10;
        o0.P[i][2] = p20;
        o0.P[i][3] = p30;
      
        o1.P[i][0] = p30;
        o1.P[i][1] = p21;
        o1.P[i][2] = p12;
        o1.P[i][3] = patch.P[i][3];
    }
}



void pisplit_patch(const BezierPatch& patch,
                   BezierPatch& o0, BezierPatch& o1,
                   const mat4& proj)
{
    float a = 0, b = 0;

    BezierPatch ppatch;
    
    transform_patch(patch, proj, ppatch);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            a += glm::distance(vec2(ppatch.P[j][i]), vec2(ppatch.P[j+1][i]));
            b += glm::distance(vec2(ppatch.P[i][j]), vec2(ppatch.P[i][j+1]));
        }
    }

    if (a > b) {
        vsplit_patch(patch, o0, o1);
    } else {
        hsplit_patch(patch, o0, o1);
    }
}


void isplit_patch(const BezierPatch& patch,
                  BezierPatch& o0, BezierPatch& o1)
{
    float a = 0, b = 0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            a += glm::distance(vec2(patch.P[j][i]), vec2(patch.P[j+1][i]));
			b += glm::distance(vec2(patch.P[i][j]), vec2(patch.P[i][j+1]));
        }
    }

    if (a > b) {
        vsplit_patch(patch, o0, o1);
    } else {
        hsplit_patch(patch, o0, o1);
    }
}


void qsplit_patch(const BezierPatch& patch, 
                  BezierPatch& o0, 
                  BezierPatch& o1, 
                  BezierPatch& o2, 
                  BezierPatch& o3) 
{
    BezierPatch t0, t1;

    hsplit_patch(patch, t0, t1);

    vsplit_patch(t0, o0, o1);
    vsplit_patch(t1, o2, o3);
}

void calc_bbox(const BezierPatch& patch, BBox& box) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            box.add_point(patch.P[i][j]);
        }
    }
}
