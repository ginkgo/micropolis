
#include "patch.h"

// Compile time constants:
// CULL_RIBBON     - float
// SCREEN_SIZE     - int2
// MAX_SPLIT_DEPTH - int
// BOUND_N_SPLIT_WORK_GROUP_SIZE - int

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


// Returns 0bABC
// A ... draw
// B ... split
// C ... horizontal/vertical
inline char bound(const global float4* patch_buffer,
                  int rpid, float2 rmin, float2 rmax)
{
    return 1;
}


// Calculate (inclusive) prefix sum
inline int prefix_sum(int v, local int* pad)
{
    size_t lid = get_local_id(0);
    //local int pad[BOUND_N_SPLIT_WORK_GROUP_SIZE];

    pad[lid] = v;

    #warning TODO
    barrier(CLK_LOCAL_MEM_FENCE);

    return pad[lid];
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
                   volatile global int* out_range_count)
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
    local int regex_pad[BOUND_N_SPLIT_WORK_GROUP_SIZE];
    
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
            } else {
                cnt = 0;
            }
        }
    
        barrier(CLK_LOCAL_MEM_FENCE);

        if (cnt == 0 && stack_height == 0) {
            return; // Global exit test
        }
    
        if (lid < cnt) {
            rpid = in_pids[start + lid];
            rmin = in_mins[start + lid];
            rmax = in_maxs[start + lid];
            occupied = 1;
        } else if (lid < cnt + stack_height) {
            rpid = pid_stack[cnt + lid];
            rmin = min_stack[cnt + lid];
            rmax = max_stack[cnt + lid];
            occupied = 1;
        } else {
            occupied = 0;
        }

        char bound_flags = 0;

        if (occupied) {
            bound_flags = bound(patch_buffer, rpid, rmin, rmax);
        }

        // Move bounded ranges to output buffer
        int sum = prefix_sum((bound_flags & 1) >> 0, regex_pad);

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
        sum = prefix_sum((bound_flags & 2) >> 1, regex_pad);

        if (lid + 1 == BOUND_N_SPLIT_WORK_GROUP_SIZE) {
            cnt = sum;
            start = stack_height - 2;
            stack_height += cnt + 2;
#warning handle local stack overflow
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        if (bound_flags & 2 != 0) {
            pid_stack[start + sum * 2 + 0] = rpid+1;
            pid_stack[start + sum * 2 + 1] = rpid+1;
            float2 c = (rmin+rmax)*0.5;

            // Check split direction
            if (bound_flags & 4) {
                // Horizontal
                min_stack[start + sum * 2 + 0] = (float2)(rmin.x, rmin.y);
                max_stack[start + sum * 2 + 0] = (float2)(rmax.x, c.y);
            
                min_stack[start + sum * 2 + 1] = (float2)(rmin.x, c.y);
                max_stack[start + sum * 2 + 1] = (float2)(rmax.x, rmax.y);
            } else {
                // Vertical
                min_stack[start + sum * 2 + 0] = (float2)(rmin.x, rmin.y);
                max_stack[start + sum * 2 + 0] = (float2)(c.x, rmax.y);
            
                min_stack[start + sum * 2 + 1] = (float2)(c.x, rmin.y);
                max_stack[start + sum * 2 + 1] = (float2)(rmax.x, rmax.y);
            }
        }

    }
    
    
}
                  
