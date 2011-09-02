
inline int calc_framebuffer_pos(int2 pxlpos)
{
    int2 gridpos = pxlpos / TILE_SIZE;
    int  grid_id = gridpos.x + GRID_SIZE.x * gridpos.y;
    int2 loclpos = pxlpos - gridpos * TILE_SIZE;
    int  locl_id = loclpos.x + TILE_SIZE * loclpos.y;

    return grid_id * TILE_SIZE * TILE_SIZE + locl_id;
}

float4 eval_spline(float4 p0, float4 p1, float4 p2, float4 p3, float t)
{
    float s = 1-t;
    return s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;
}

float4 eval_patch(private float4* patch, float2 st)
{
    float4 P[4];

    for (int i = 0; i < 4; ++i) {
        P[i] = eval_spline(patch[4 * i + 0],
                           patch[4 * i + 1],
                           patch[4 * i + 2],
                           patch[4 * i + 3], st.y);
    }

    return eval_spline(P[0],P[1],P[2],P[3], st.x);
}

float4 mul_m44v4(float16 mat, float4 vec)
{
    return (float4) {dot(mat.s048C, vec),
                     dot(mat.s159D, vec),
                     dot(mat.s26AE, vec),
                     dot(mat.s37BF, vec)};
            
}

constant sampler_t fbsampler = CLK_NORMALIZED_COORDS_FALSE |
                               CLK_ADDRESS_CLAMP |
                               CLK_FILTER_NEAREST;

inline size_t calc_grid_pos(size_t nu, size_t nv, size_t patch, size_t w, size_t h)
{
    return nu + nv * w + patch * w * h;
}

__kernel void dice (const global float4* patch_buffer, // 0
                    global float4* grid_buffer)        // 1
{
    float4 patch[16];

    if (get_global_id(0) > PATCH_SIZE || 
        get_global_id(1) > PATCH_SIZE) {
        return;
    }

    size_t nv = get_global_id(0), nu = get_global_id(1);
    size_t patch_id = get_global_id(2);

    size_t w = PATCH_SIZE+1, h = PATCH_SIZE+1;

    for (int i = 0; i < 16; ++i) {
        patch[i] = patch_buffer[patch_id * 16 + i];
    }

    float2 uv = (float2) {nu/(float)(128), nv/(float)(128)};

    float4 pos = eval_patch(patch, uv);

    grid_buffer[calc_grid_pos(nu, nv, patch_id, w, h)] = pos;
}

__kernel void shade(const global float4* grid_buffer, // 0
                    global float4* framebuffer,       // 1
                    float16 proj,                     // 2
                    int4 viewport)                    // 3
{
    float4 pos[2][2];

    size_t w = PATCH_SIZE+1, h = PATCH_SIZE+1;

    int nv = get_global_id(0), nu = get_global_id(1);
    int patch_id = get_global_id(2);

    for     (int vi = 0; vi < 2; ++vi) {
        for (int ui = 0; ui < 2; ++ui) {
            pos[ui][vi] = grid_buffer[calc_grid_pos(nu+ui, nv+vi, patch_id, w, h)];
        }
    }

    float3 du = (pos[1][0] - pos[0][0] + pos[1][1] - pos[0][1]).xyz * 0.5f;
    float3 dv = (pos[0][1] - pos[0][0] + pos[1][1] - pos[1][0]).xyz * 0.5f;

    float3 n = normalize(cross(du,dv));

    float4 p = mul_m44v4(proj, pos[0][0]);

    if (p.w < 0) return;

    int2 coord = {(int)(p.x/p.w * viewport.z/2 + viewport.z/2),
                  (int)(p.y/p.w * viewport.w/2 + viewport.w/2)};
    
    if (coord.x < 0 || coord.x >= viewport.z || 
        coord.y < 0 || coord.y >= viewport.w)
        return;

    int ipos = calc_framebuffer_pos(coord);

    framebuffer[ipos] = (float4){n.x, n.y, n.z, 1};
}
                    

/* struct triangle_buffer */
/* { */
/*     float16 Ax, Ay, Ad; */
/*     float16 Bx, By, Bd; */

/*     float16 D1, D2, D3; */

/*     float4 color[16]; */
/* }; */

/* struct index_elem */
/* { */
/*     int next; */
/*     size_t start, end; */
/* }; */

/* __kernel void hider (__global triangle_block* triangle_buffer, */
/*                      __global int* index_heads, __global index_elem* index_buffer, */
/*                      __global float4* framebuffer, __global float* depthbuffer, */
/*                      int bsize, int2 gridsize) */
/* { */
/*     __local index_elem block; */
/*     __local triangle_block triangles; */
/*     __local block_event, event; */

/*     int2 tile_id = {get_group_id(0), get_group_id(1)};    */
/*     int tilepos = tile_id.x + gridsize.x * tile_id.y; */

/*     float2 global_id = {get_global_id(0), get_global_id(1)}; */
/*     float2 pos = {(float)global_id.x, (float)global_id.y}; */
/*     int fbpos = calc_framebuffer_pos(global_id, bsize, gridsize); */

/*     float4 color = framebuffer[fbpos]; */
/*     float depth = depthbuffer[fbpos]; */

/*     int block_id = index_heads[tilepos]; */

/*     while (block_id >= 0) { */
/*         block_event = async_work_group_copy(&block, index_buffer + block_id, 1, 0); */
/*         wait_group_events(1, &block_event); */

/*         size_t start_block = block->start; */
/*         size_t end_block = block->end; */
     
/*         for (size_t i = start_block; i < end_block; ++i) { */

/*             event = async_work_group_copy(&triangle_cache, */
/*                                           triangle_buffer + i, 1, 0); */
/*             wait_group_events(1, &event); */
        
/*             float16 S = (triangles->Ax * pos.x +  */
/*                          triangles->Ay * pos.y +  */
/*                          triangles->Ad); */
/*             float16 T = (triangles->Bx * pos.x +  */
/*                          triangles->By * pos.y +  */
/*                          triangles->Bd); */
/*             float16 R = 1 - (S + T); */

/*             bool16 mask = S < 1 && S > 0 && T < 1 && T > 0 && R < 1 && R > 0; */

/*             float16 D = triangles->D1 * S + triangles->D2 * T + triangles->D3 * R; */

/*             for (int t = 0; t < 16; ++t) { */
/*                 if (mask[t] && D[t] < depth) { */
/*                     depth = D[t]; */
/*                     color = triangles->color[t]; */
/*                 } */
/*             } */
/*         } */
        
/*         block_id = block->next; */
/*     } */

/*     framebuffer[fbpos] = color; */
/*     depthbuffer[fbpos] = depth; */

/* } */
