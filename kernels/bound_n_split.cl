
#include "utility.h"


// Compile time constants:
// CULL_RIBBON                   - float
// BOUND_N_SPLIT_WORK_GROUP_SIZE - int
// BOUND_SAMPLE_RATE             - int
// MAX_SPLIT_DEPTH               - int
// BOUND_N_SPLIT_LIMIT           - float


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
char bound(const global float4* patch_buffer,
           int rpid, float2 rmin, float2 rmax, int rdepth,
           constant const matrix4* mv, constant const projection* P, float split_limit)
{
    // Calculate bounding box and max u/v length of patch 
    float2 ppos[RES][RES];
    
    float3 bbox_min = (float3)( INFINITY, INFINITY, INFINITY);
    float3 bbox_max = (float3)(-INFINITY,-INFINITY,-INFINITY);
    
    for (size_t u = 0; u < RES; ++u) {
        for (size_t v = 0; v < RES; ++v) {
            float2 uv = mix(rmin, rmax, (float2)(u * (1.0f / (RES-1)), v * (1.0f / (RES-1))));

            float4 p = eval_patch(patch_buffer, rpid, uv);
            p = mul_cm4v4(mv, p);

            bbox_min = min(bbox_min, p.xyz);
            bbox_max = max(bbox_max, p.xyz);
            
            p = mul_cm4v4(&P->proj, p);
            ppos[v][u] = mul_cm2v2(&P->screen_matrix, p.xy / p.w);            
        }
    }

    bool vsplit = 0;
    
    if (outside_frustum(bbox_min, bbox_max, P)) {
        return 0; // CULL
    } else if (bbox_min.z < 0 && bbox_max.z > 0) {
        if (rdepth >= MAX_SPLIT_DEPTH) {
            return 0; // CULL
        } else {
            // Pick split direction from parameter space
            vsplit = (rmax.x - rmin.x) < (rmax.y - rmin.y);
        } 
    } else {

        float hlen=0, vlen=0;
        for (size_t i = 0; i < RES; ++i) {
            float h = 0, v = 0;
            for (size_t j = 0; j < RES; ++j) {
                h += distance(ppos[i][j], ppos[i][j+1]);
                v += distance(ppos[j][i], ppos[j+1][i]);
            }
        
            hlen = max(h, hlen);
            vlen = max(v, vlen);
        }

        if (hlen <= split_limit && vlen <= split_limit) {
            return 1; // DRAW
        } else if (rdepth >= MAX_SPLIT_DEPTH-2) {
            // Max split depth reached
            return 0; // CULL
        } else {
            vsplit = hlen < vlen;
        }
    }
    
    return 2 & (vsplit ? 3 : 0); // SPLIT
}




kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1, 1)))
void bound_n_split(const global float4* patch_buffer,
                    
                   global int* in_pids,
                   global float2* in_mins,
                   global float2* in_maxs,
                   volatile global int* in_range_cnt,
           
                   global int* out_pids,
                   global float2* out_mins,
                   global float2* out_maxs,
                   volatile global int* out_range_count,

                   matrix4 modelview,
                   constant const projection* proj)
{
    const size_t lid = get_local_id(0);

    // stack properties
    int occupied;
    int rpid;
    float2 rmin, rmax;

    // local stack
    local int stack_height;
    local int pid_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * (MAX_SPLIT_DEPTH + 1)];
    local float2 min_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * (MAX_SPLIT_DEPTH + 1)];
    local float2 max_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * (MAX_SPLIT_DEPTH + 1)];

    
    // pad for prefix sum
    local int prefix_pad[BOUND_N_SPLIT_WORK_GROUP_SIZE];
    
    // Number of items to be copied from input buffer
    local int cnt, start;

    if (lid == 0) {
        stack_height = 0;
        cnt = 0;
        start = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    
    while (1) {
        // Load range elements (source from stack first and fall back to input buffer if necessary)
        if (lid == 0) {
            if (stack_height < BOUND_N_SPLIT_WORK_GROUP_SIZE) {
                int top = atomic_sub(in_range_cnt, BOUND_N_SPLIT_WORK_GROUP_SIZE - stack_height);
                start = top - BOUND_N_SPLIT_WORK_GROUP_SIZE - stack_height;
                cnt = max(BOUND_N_SPLIT_WORK_GROUP_SIZE - stack_height, min(0, top));

                #warning todo: update stack_height
            } else {
                cnt = 0;
            }
        }
    
        barrier(CLK_LOCAL_MEM_FENCE);

        if (cnt == 0 && stack_height == 0) {
            return; // Global exit test
        }

#warning TODO: Handle stack_height stuff properly
        if (lid < cnt) {
            rpid = in_pids[start + lid];
            rmin = in_mins[start + lid];
            rmax = in_maxs[start + lid];
            occupied = 1;
        } else if (lid < cnt + stack_height) {
            rpid = pid_stack[stack_height + lid - cnt];
            rmin = min_stack[stack_height + lid - cnt];
            rmax = max_stack[stack_height + lid - cnt];
            occupied = 1;
        } else {
            occupied = 0;
        }

        char bound_flags = 0;

        if (occupied) {
            bound_flags = 1; //bound(patch_buffer, rpid, rmin, rmax);
        }

        // Move bounded ranges to output buffer
        int sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 1) >> 0, prefix_pad);

        if (lid + 1 == BOUND_N_SPLIT_WORK_GROUP_SIZE) {
            cnt = sum;
            start = atomic_add(out_range_count, cnt) - 1;
#warning handle overflow
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        if (bound_flags & 1 != 0) {
            out_pids[start + sum] = rpid;
            out_mins[start + sum] = rmin;
            out_maxs[start + sum] = rmax;
        }

        // Perform split
        sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 2) >> 1, prefix_pad);

        if (lid + 1 == BOUND_N_SPLIT_WORK_GROUP_SIZE) {
            cnt = sum;
            start = stack_height - 2;
            stack_height += cnt + 2;
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        if (bound_flags & 2 != 0) {
            pid_stack[start + sum * 2 + 0] = rpid+1;
            pid_stack[start + sum * 2 + 1] = rpid+1;
            float2 c = (rmin+rmax)*0.5;

            // Check split direction
            if (bound_flags & 4) {
                // Vertical
                min_stack[start + sum * 2 + 0] = (float2)(rmin.x, rmin.y);
                max_stack[start + sum * 2 + 0] = (float2)(c.x, rmax.y);
            
                min_stack[start + sum * 2 + 1] = (float2)(c.x, rmin.y);
                max_stack[start + sum * 2 + 1] = (float2)(rmax.x, rmax.y);
            } else {
                // Horizontal
                min_stack[start + sum * 2 + 0] = (float2)(rmin.x, rmin.y);
                max_stack[start + sum * 2 + 0] = (float2)(rmax.x, c.y);
            
                min_stack[start + sum * 2 + 1] = (float2)(rmin.x, c.y);
                max_stack[start + sum * 2 + 1] = (float2)(rmax.x, rmax.y);
            }
        }

    }
    
    
}
                  
                  

kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1,1)))
void init_range_buffers(global int* pids,
                        global float2* mins,
                        global float2* maxs,
                        int range_count)
{
    const int gid = get_local_id(0);

    if (gid < range_count) {
        pids[gid] = gid;
        mins[gid] = (float2)(0,0);
        maxs[gid] = (float2)(1,1);
    }
}
