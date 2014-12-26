#include "utility.h"

// Compile time constants:
// PATCH_SIZE            - int
// TILE_SIZE             - int
// GRID_SIZE             - int2
// VIEWPORT_MIN_PIXEL    - int2
// VIEWPORT_MAX_PIXEL    - int2
// VIEWPORT_SIZE_PIXEL   - int2
// MAX_BLOCK_ASSIGNMENTS - int
// DISPLACEMENT          - int(bool)

#define BLOCKS_PER_LINE (PATCH_SIZE/8)
#define BLOCKS_PER_PATCH (BLOCKS_PER_LINE*BLOCKS_PER_LINE)

#define VIEWPORT_MIN  (VIEWPORT_MIN_PIXEL  << PXLCOORD_SHIFT)
#define VIEWPORT_MAX  ((VIEWPORT_MAX_PIXEL << PXLCOORD_SHIFT) - 1)
#define VIEWPORT_SIZE (VIEWPORT_SIZE_PIXEL << PXLCOORD_SHIFT)


size_t calc_grid_pos(size_t nu, size_t nv, size_t patch);


__kernel void dice (const global float4* patch_buffer,
                    const global int* pid_buffer,
                    const global float2* min_buffer,
                    const global float2* max_buffer,
                    global float4* pos_grid,
                    global int2* pxlpos_grid,
                    global float* depth_grid,
                    float16 modelview,
                    float16 proj)
{
    size_t nv = get_global_id(0), nu = get_global_id(1);
    size_t range_id = get_global_id(2);
    size_t patch_id = pid_buffer[range_id];

    if (nv > PATCH_SIZE || nu > PATCH_SIZE) return;
        
    float2 rmin = min_buffer[get_global_id(2)];
    float2 rmax = max_buffer[get_global_id(2)];
    
    float2 uv = (float2)(mix(rmin, rmax, (float2)(nu/(float)PATCH_SIZE, nv/(float)PATCH_SIZE)));

    float4 pos = mul_m44v4(modelview, eval_patch(patch_buffer, patch_id, uv));

    if (DISPLACEMENT) {
        const float f1=0.04f;
        const float f2=0.02f;
        const float f3=0.01f;
        
        pos.x += native_sin(pos.y*5*2) * f1;
        pos.y += native_sin(pos.x*5*2) * f1;
        pos.z += native_sin(pos.y*5*2) * native_sin(pos.x*3*2) * f1;
        
        pos.x += native_sin(pos.y*7*2) * f2;
        pos.y += native_sin(pos.x*7*2) * f2;
        pos.z += native_sin(pos.y*7*2) * native_sin(pos.x*3*2) * f2;
        
        pos.x += native_sin(pos.y*11*2) * f3;
        pos.y += native_sin(pos.x*11*2) * f3;
        pos.z += native_sin(pos.y*11*2) * native_sin(pos.x*3*2) * f3;
    }
    
    float4 p = mul_m44v4(proj, pos);

    int2 coord = (int2)((int)(p.x/p.w * VIEWPORT_SIZE.x/2 + VIEWPORT_SIZE.x/2),
                        (int)(p.y/p.w * VIEWPORT_SIZE.y/2 + VIEWPORT_SIZE.y/2));


    int grid_index = calc_grid_pos(nu, nv, range_id);
    
    pos_grid[grid_index] = pos;
    pxlpos_grid[grid_index] = coord;
    depth_grid[grid_index] = p.z/p.w;
}
