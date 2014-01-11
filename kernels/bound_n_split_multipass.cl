
#include "utility.h"
#include "bound_n_split.h"

kernel void bound_kernel(const global float4* patch_buffer,
                         int batch_size,
                         int batch_offset,
                         
                         global const int* pids,
                         global const uchar* depths,
                         global const float2* mins,
                         global const float2* maxs,

                         global uchar* bound_flags,
                         global int* split_flags,
                         global int* draw_flags,
                         
                         global int* pid_pad,
                         global uchar* depth_pad,
                         global float2* min_pad,
                         global float2* max_pad,
                         
                         matrix4 modelview,
                         constant const projection* proj,
                         float split_limit)
{
    int lid = get_global_id(0);
    int gid = lid + batch_offset;

    if (lid >= batch_size) return;

    int rpid = pids[gid];
    uchar rdepth = depths[gid];
    float2 rmax = maxs[gid];
    float2 rmin = mins[gid];
    
    uchar flags = bound(patch_buffer,
                        rpid, rmin, rmax, rdepth,
                        &modelview, proj, split_limit);

    bound_flags[lid] = flags;

    draw_flags[lid]  = (flags>>0)&1;
    split_flags[lid] = (flags>>1)&1;

    pid_pad[lid] = rpid;
    depth_pad[lid] = rdepth+1;
    min_pad[lid] = rmin;
    max_pad[lid] = rmax;
}



kernel void move(int batch_size,
                 int batch_offset,

                 global const int* pid_pad,
                 global const uchar* depth_pad,
                 global const float2* min_pad,
                 global const float2* max_pad,

                 
                 global const uchar* bound_flags,
                 global const int* draw_sum,
                 global const int* split_sum,
                 
                 global int* pid_stack,
                 global uchar* depth_stack,
                 global float2* min_stack,
                 global float2* max_stack,
                 
                 global int* out_pids,
                 global float2* out_mins,
                 global float2* out_maxs)
{
    int lid = get_global_id(0);

    if (lid >= batch_size) return;
    
    uchar bound_flag = bound_flags[lid];

    int rpid     = pid_pad[lid];
    uchar rdepth = depth_pad[lid];
    float2 rmin  = min_pad[lid];
    float2 rmax  = max_pad[lid];
    
    if ((bound_flag>>0)&1) {
        // DRAW
        // Move range to output buffer

        int pos = draw_sum[lid] - 1;

        out_pids[pos] = rpid;
        out_mins[pos] = rmin;
        out_maxs[pos] = rmax;
        
    } else if ((bound_flag>>1)&1) {
        // SPLIT
        // Move patches back on stack

        int pos0 = batch_offset + (split_sum[lid] - 1) * 2 + 0;
        int pos1 = pos0 + 1;
        
        pid_stack[pos0] = rpid;
        pid_stack[pos1] = rpid;

        depth_stack[pos0] = rdepth;
        depth_stack[pos1] = rdepth;

        float2 c = (rmin+rmax)*0.5f;
        
        min_stack[pos0] = rmin;
        max_stack[pos1] = rmax;
        
        // Check split direction
        if (bound_flag & 4) {
            // Vertical split
            max_stack[pos0] = (float2)(c.x, rmax.y);
            min_stack[pos1] = (float2)(c.x, rmin.y);
        } else {
            // Vertical split
            max_stack[pos0] = (float2)(rmax.x, c.y);
            min_stack[pos1] = (float2)(rmin.x, c.y);
        }
    }
}
