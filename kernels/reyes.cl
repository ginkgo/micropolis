
int calc_framebuffer_pos(int2 pxlpos, int bsize, int2 gridsize)
{
    int2 gridpos = pxlpos / bsize;
    int  grid_id = gridpos.x + gridsize.x * gridpos.y;
    int2 loclpos = pxlpos - gridpos * bsize;
    int  locl_id = loclpos.x + bsize * loclpos.y;

    return grid_id * bsize * bsize + locl_id;
}

__kernel void clear (__global float4* framebuffer, float4 color,
                     int bsize, int2 gridsize)
{
    int2   pxlpos  = (int2)  {get_global_id(0),   get_global_id(1)};
 
    int pos = calc_framebuffer_pos(pxlpos, bsize, gridsize);

    framebuffer[pos] = color;
}

float4 eval_spline(float4 p0, float4 p1, float4 p2, float4 p3, float t)
{
    float s = 1-t;
    return s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;
}

float4 eval_patch(__local float4* patch, float2 st)
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

const sampler_t fbsampler = CLK_NORMALIZED_COORDS_FALSE | 
                            CLK_ADDRESS_CLAMP |
                            CLK_FILTER_NEAREST;

__kernel void dice (__global float4* patch_buffer,
                    __global float4* framebuffer, int bsize, int2 gridsize,
                    float16 proj, int4 viewport)
{
    __local float4 patch[16];

    int2 vertex_id = {get_global_id(0), get_global_id(1)};

    float2 st = {vertex_id.x/(float)(get_global_size(0)-1),
                 vertex_id.y/(float)(get_global_size(1)-1)};
    int patch_id = get_global_id(2);

    event_t event;
    event = async_work_group_copy(patch, patch_buffer + 16 * patch_id, 16, 0);
    wait_group_events(1, &event);
           
    float4 pos = eval_patch(patch, st);

    pos = mul_m44v4(proj, pos);

    int2 coord = {(int)(pos.x/pos.w * viewport.z/2 + viewport.z/2), 
                  (int)(pos.y/pos.w * viewport.w/2 + viewport.w/2)};
    
    if (coord.x < 0 || coord.y < 0 || 
        coord.x >= viewport.z || coord.y >= viewport.w)
        return;

    float depth = pos.w;

    int ipos = calc_framebuffer_pos(coord, bsize, gridsize);

    if (framebuffer[ipos].w > depth) {
        framebuffer[ipos] =  (float4){st.x,st.y,0,depth};
    }
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
