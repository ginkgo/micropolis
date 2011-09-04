
// Compile time constants:
// PATCH_SIZE            - int
// TILE_SIZE             - int
// GRID_SIZE             - int2
// VIEWPORT_MIN          - int2
// VIEWPORT_MAX          - int2
// VIEWPORT_SIZE         - int2
// MAX_BLOCK_COUNT       - int
// MAX_BLOCK_ASSIGNMENTS - int

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

inline size_t calc_grid_pos(size_t nu, size_t nv, size_t patch)
{
    return nu + nv * (PATCH_SIZE+1) + patch * (PATCH_SIZE+1)*(PATCH_SIZE+1);
}

__kernel void dice (const global float4* patch_buffer,
                    global float4* pos_grid,
                    global int2* pxlpos_grid,
                    float16 proj)
{
    float4 patch[16];

    if (get_global_id(0) > PATCH_SIZE ||
        get_global_id(1) > PATCH_SIZE) {
        return;
    }

    size_t nv = get_global_id(0), nu = get_global_id(1);
    size_t patch_id = get_global_id(2);

    for (int i = 0; i < 16; ++i) {
        patch[i] = patch_buffer[patch_id * 16 + i];
    }

    float2 uv = (float2) {nu/(float)PATCH_SIZE, nv/(float)PATCH_SIZE};

    float4 pos = eval_patch(patch, uv);

    float4 p = mul_m44v4(proj, pos);

    int2 coord = {(int)(p.x/p.w * VIEWPORT_SIZE.x/2 + VIEWPORT_SIZE.x/2),
                  (int)(p.y/p.w * VIEWPORT_SIZE.y/2 + VIEWPORT_SIZE.y/2)};

    
    int grid_index = calc_grid_pos(nu, nv, patch_id);
    pos_grid[grid_index] = pos;
    pxlpos_grid[grid_index] = coord;
}


inline int is_front_facing(const int2 *ps)
{
    int2 d1 = ps[1] - ps[0];
    int2 d2 = ps[2] - ps[0];
    int2 d3 = ps[3] - ps[0];

    return 1;//(d1.x*d3.y-d3.x*d1.y > 0 || d3.x*d2.y-d2.x*d3.y > 0);
}

inline int is_nonempty(int2 min, int2 max)
{
    return 1;//min.x < max.x && min.y < max.y;
}

inline int calc_block_pos(int u, int v, int patch_id)
{
    return u + v * (PATCH_SIZE/8) + patch_id * (PATCH_SIZE/8) * (PATCH_SIZE/8);
}

__kernel void shade(const global float4* pos_grid,
                    const global int2* pxlpos_grid,
                    global int4* block_index)
{
    volatile __local int x_min;
    volatile __local int y_min;
    volatile __local int x_max;
    volatile __local int y_max;

    if (get_local_id(0) == 0 && get_local_id(1) == 0) {
        x_min = VIEWPORT_MAX.x;
        y_min = VIEWPORT_MAX.y;
        x_max = VIEWPORT_MIN.x;
        y_max = VIEWPORT_MIN.y;
    }

    // V
    // |
    // 2 - 3 
    // | / |
    // 0 - 1 - U

    float4 pos[4];
    int2 pxlpos[4];

    int nv = get_global_id(0), nu = get_global_id(1);
    int patch_id = get_global_id(2);

    int2 pmin = VIEWPORT_MAX;
    int2 pmax = VIEWPORT_MIN;

    for     (int vi = 0; vi < 2; ++vi) {
        for (int ui = 0; ui < 2; ++ui) {
            int i = ui + vi * 2;
            pos[i] = pos_grid[calc_grid_pos(nu+ui, nv+vi, patch_id)];
            int2 p  = pxlpos_grid[calc_grid_pos(nu+ui, nv+vi, patch_id)];
            
            pmin = min(pmin, p);
            pmax = max(pmax, p);

            pxlpos[i] = p;
        }
    }

    if (is_front_facing(pxlpos) && is_nonempty(pmin, pmax)) {
        atomic_min(&x_min, pmin.x); 
        atomic_min(&y_min, pmin.y);
        atomic_max(&x_max, pmax.x); 
        atomic_max(&y_max, pmax.y);

        /* float3 du = (pos[1] - pos[0] +  */
        /*              pos[3] - pos[2]).xyz * 0.5f; */
        /* float3 dv = (pos[2] - pos[0] +  */
        /*              pos[3] - pos[1]).xyz * 0.5f; */

        /* float3 n = normalize(cross(du,dv)) * 0.5f + 0.5f; */
    }

    if (get_local_id(0) == 0 &&  get_local_id(1) == 0) {
        int i = calc_block_pos(get_group_id(0), get_group_id(1), get_group_id(2));
        block_index[i] = (int4){x_min, y_min, x_max, y_max};
    }   
}

__kernel void clear_heads(global int* heads)
{
    heads[get_global_id(0)] = -1;
}

inline int calc_node_pos(int block_id, int assign_cnt)
{
    if (assign_cnt >= MAX_BLOCK_ASSIGNMENTS) {
        return -2;
    }
    
    return block_id + assign_cnt * MAX_BLOCK_COUNT;
}

inline int calc_tile_id(int tx, int ty)
{
    return tx + FRAMEBUFFER_SIZE.x/8 * ty;
}

__kernel void assign(global const int4* block_index,
                     volatile global int* heads,
                     global int2* node_heap)
{
    int block_id = get_global_id(0);
    int4 block_bound = block_index[block_id];

    int assign_cnt = 0;
    
    if (!is_nonempty(block_bound.xy, block_bound.zw)) {
        // Empty block
        return;
    }

    int2 min_tile = block_bound.xy / 8;
    int2 max_tile = block_bound.zw / 8;

    for     (int ty = min_tile.y; ty <= max_tile.y; ++ty) {
        for (int tx = min_tile.x; tx <= max_tile.x; ++tx) {
            int tile_id = calc_tile_id(tx,ty);
            int heap_pos = calc_node_pos(block_id, assign_cnt);

            ++assign_cnt;

            int old_head = atomic_xchg(heads + tile_id, heap_pos);

            if (heap_pos >= 0) {
                node_heap[heap_pos] = (int2){block_id, old_head};
            }
        }
    }
    
}


__kernel void sample(global const int* heads,
                     global const int2* node_heap,
                     global float4* framebuffer)
{
    int tile_id  = calc_tile_id(get_group_id(0), get_group_id(1)); 
    int next = heads[tile_id];

    /* if (get_local_id(0) == 0 && get_local_id(1) == 0) { */
    /*     tile_id; */
    /*     next = heads[t; */
    /* } */

    int cnt = 1;

    while (next >= 0) {
        int2 node = node_heap[next];
        
        ++cnt;

        next = node.y;
    }

    int fb_id = calc_framebuffer_pos((int2){get_global_id(0), get_global_id(1)});
    if (next == -2) {
        framebuffer[fb_id] = (float4){1,0,0,1};
    } else {
        /* framebuffer[fb_id] = (float4){0,1,0,1}; */
       
        float v = 1.0f - (1.0f / cnt);

        framebuffer[fb_id] = (float4){v,v,v,1};     
    }
}
                     
