
#include "utility.h"

// Compile time constants:
// CULL_RIBBON                   - float


bool outside_frustum(float3 pmin, float3 pmax, constant const projection* P)
{
    float2 smin,smax;

    float n = -pmax.z;
    float f = -pmin.z;

    if (f < P->near || n > P->far) {
        return true;
    }

    n = max(P->near, n);

    smin.x = pmin.x/((pmin.x < 0) ? n : f) * P->f.x + P->screen_size.x * 0.5;
    smin.y = pmin.y/((pmin.y < 0) ? n : f) * P->f.y + P->screen_size.y * 0.5;

    smax.x = pmax.x/((pmax.x > 0) ? n : f) * P->f.x + P->screen_size.x * 0.5;
    smax.y = pmax.y/((pmax.y > 0) ? n : f) * P->f.y + P->screen_size.y * 0.5;

    return (smin.x > P->screen_size.x-1 + CULL_RIBBON || smax.x < -CULL_RIBBON ||
            smin.y > P->screen_size.y-1 + CULL_RIBBON || smax.y < -CULL_RIBBON );
}


// Returns 0b00000CBA
// A ... draw
// B ... split
// C ... split direction, 0=horizontal 1=vertical
#define RES BOUND_SAMPLE_RATE
#define CULL 0
#define DRAW 1
#define HSPLIT 2
#define VSPLIT 6
uchar bound(const global float4* patch_buffer,
           int rpid, float2 rmin, float2 rmax, uchar rdepth,
           private const matrix4* mv, constant const projection* P, float split_limit)
{   
    // Calculate bounding box and max u/v length of patch 
    float2 ppos[RES][RES];
    
    float3 bbox_min = (float3)( INFINITY, INFINITY, INFINITY);
    float3 bbox_max = (float3)(-INFINITY,-INFINITY,-INFINITY);

    uchar mask = (rdepth >= MAX_SPLIT_DEPTH-1) ? 0 : 0xff;
    
    for (size_t u = 0; u < RES; ++u) {
        for (size_t v = 0; v < RES; ++v) {
            float2 uv = mix(rmin, rmax, (float2)(u * (1.0f / (RES-1)), v * (1.0f / (RES-1))));

            float4 p = eval_patch(patch_buffer, rpid, uv);
            p = mul_pm4v4(mv, p);

            bbox_min = min(bbox_min, p.xyz);
            bbox_max = max(bbox_max, p.xyz);
            
            p = mul_cm4v4(&P->proj, p);
            ppos[v][u] = mul_cm2v2(&P->screen_matrix, p.xy / p.w);            
        }
    }

    if (outside_frustum(bbox_min, bbox_max, P)) {
        return CULL;
    } else if (bbox_min.z < 0 && bbox_max.z > 0) {
        return (((rmax.x - rmin.x) < (rmax.y - rmin.y)) ? VSPLIT : HSPLIT) & mask;
    } else {

        float hlen=0, vlen=0;
        for (size_t i = 0; i < RES; ++i) {
            float h = 0, v = 0;
            for (size_t j = 0; j < RES-1; ++j) {
                h += distance(ppos[i][j], ppos[i][j+1]);
                v += distance(ppos[j][i], ppos[j+1][i]);
            }
        
            hlen = max(h, hlen);
            vlen = max(v, vlen);
        }

        if (hlen <= split_limit && vlen <= split_limit) {
            return DRAW;
        } else {
            return ((hlen > vlen) ? VSPLIT : HSPLIT) & mask;
        }
    }

    // unreachable
}

