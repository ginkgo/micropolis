
#include "patch.h"

// Compile time constants:
// CULL_RIBBON   - float
// SCREEN_SIZE   - int2

bool intersects_frustum(float3 pmin, float3 pmax, float near, float far, float2 proj_f)
{
    float2 smin,smax;

    float n = -pmax.z;
    float f = -pmin.z;

    if (f < near || n > far) {
        return false;
    }

    n = max(near, n);

    if (pmin.x < 0) {
        smin.x = pmin.x/n * proj_f.x + SCREEN_SIZE.x * 0.5;
    } else {
        smin.x = pmin.x/f * proj_f.x + SCREEN_SIZE.x * 0.5;
    }

    if (pmin.y < 0) {
        smin.y = pmin.y/n * proj_f.y + SCREEN_SIZE.y * 0.5;
    } else {
        smin.y = pmin.y/f * proj_f.y + SCREEN_SIZE.y * 0.5;
    }

    if (pmax.x > 0) {
        smax.x = pmax.x/n * proj_f.x + SCREEN_SIZE.x * 0.5;
    } else {
        smax.x = pmax.x/f * proj_f.x + SCREEN_SIZE.x * 0.5;
    }

    if (pmax.y > 0) {
        smax.y = pmax.y/n * proj_f.y + SCREEN_SIZE.y * 0.5;
    } else {
        smax.y = pmax.y/f * proj_f.y + SCREEN_SIZE.y * 0.5;
    }


    if (smin.x > SCREEN_SIZE.x-1 + CULL_RIBBON || smax.x < -CULL_RIBBON ||
        smin.y > SCREEN_SIZE.y-1 + CULL_RIBBON || smax.y < -CULL_RIBBON ){
        return false;
    }

    return true;
}

kernel void bound(global int2* needs_split)
{
    needs_split[get_global_id(0)] = (int2)(1,2);
}
                  
