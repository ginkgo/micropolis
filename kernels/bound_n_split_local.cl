
#include "utility.h"
#include "bound_n_split.h"

// Compile time constants:
// BATCH_SIZE                    - size_t
// BOUND_N_SPLIT_LIMIT           - float
// BOUND_N_SPLIT_WORK_GROUP_CNT  - int
// BOUND_N_SPLIT_WORK_GROUP_SIZE - int
// BOUND_SAMPLE_RATE             - int 
// CULL_RIBBON                   - float
// MAX_SPLIT_DEPTH               - int


kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1, 1)))
void bound_n_split(const global float4* patch_buffer,

                   int in_buffer_stride,
                   global uint* in_pids,
                   global float2* in_mins,
                   global float2* in_maxs,
                   volatile global int* in_range_cnt,
           
                   global int* out_pids,
                   global float2* out_mins,
                   global float2* out_maxs,
                   volatile global int* out_range_cnt,

                   volatile global int* processed_count,

                   matrix4 modelview,
                   constant const projection* proj,
                   float split_limit)
{
    const size_t lid = get_local_id(0);
    const size_t wid = get_global_id(1);

    // stack properties
    int occupied;
    int rpid;
    uchar rdepth;
    float2 rmin, rmax;

    // local stack
    local int stack_height;
    local int pid_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * MAX_SPLIT_DEPTH];
    local uchar depth_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * MAX_SPLIT_DEPTH];
    local float2 min_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * MAX_SPLIT_DEPTH];
    local float2 max_stack[BOUND_N_SPLIT_WORK_GROUP_SIZE * MAX_SPLIT_DEPTH];
    
    // pad for prefix sum
    local int prefix_pad[BOUND_N_SPLIT_WORK_GROUP_SIZE];
    
    // Number of items to be copied from input/output buffer copy
    local int cnt, start, top;

    // Number of items to be copied from stack
    local int stack_cnt;

    if (lid == 0) {
        stack_height = 0;
        stack_cnt = 0;
        start = 0;
        top = 0;
        cnt = 0;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);

    
    while (1) {
        // Load range elements (source from stack first and fall back to input buffer if necessary)
        if (lid == 0) {
            if (stack_height < BOUND_N_SPLIT_WORK_GROUP_SIZE) {
                cnt = min(BOUND_N_SPLIT_WORK_GROUP_SIZE - stack_height, BOUND_N_SPLIT_WORK_GROUP_SIZE);
                
                top = atomic_sub(in_range_cnt + wid, cnt);

                cnt = min(cnt, max(0, top));
                start = top - cnt;
                
                stack_cnt = min(stack_height, BOUND_N_SPLIT_WORK_GROUP_SIZE - cnt);
                stack_height -= stack_cnt;
            } else {
                start = 0;
                cnt = 0;
                
                stack_cnt = min(stack_height, BOUND_N_SPLIT_WORK_GROUP_SIZE);
                stack_height -= stack_cnt;
            }
            
            processed_count[wid] += cnt + stack_cnt;
        }
    
        barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

        if (cnt == 0 && stack_cnt == 0) {
            return; // Global exit condition
        }

        if (lid < cnt) {
            size_t pos = wid * in_buffer_stride + start + lid;
            // in_pids packs the depth into the 8 most significant bits

            uint x = in_pids[pos];
            
            rpid   = x & 0xffffff;
            rdepth = x >> 24;
            rmin   = in_mins[pos];
            rmax   = in_maxs[pos];
            occupied = 1;
        } else if (lid < cnt + stack_cnt) {
            rpid   = pid_stack[stack_height + lid - cnt];
            rdepth = depth_stack[stack_height + lid - cnt];
            rmin   = min_stack[stack_height + lid - cnt];
            rmax   = max_stack[stack_height + lid - cnt];
            occupied = 1;
        } else {
            occupied = 0;
        }

        char bound_flags = 0;

        if (occupied) {
            bound_flags = bound(patch_buffer, rpid, rmin, rmax, rdepth,
                                &modelview, proj, split_limit);
        }

        // Perform split
        int sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 2) >> 1, prefix_pad);

        if (lid == BOUND_N_SPLIT_WORK_GROUP_SIZE - 1) {

            cnt = sum;
            start = stack_height;
            stack_height += cnt * 2;

        }

        barrier(CLK_LOCAL_MEM_FENCE);

        // Prefix sum is inclusive, decrement it to get exclusive result
        sum--;
        

        if (bound_flags & 2) {
            depth_stack[start + sum * 2 + 0] = rdepth+1;
            pid_stack  [start + sum * 2 + 0] = rpid;
            
            depth_stack[start + sum * 2 + 1] = rdepth+1;
            pid_stack  [start + sum * 2 + 1] = rpid;
            
            float2 c = (rmin+rmax)*0.5f;

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

        // Move bounded ranges to output buffer
        sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 1) >> 0, prefix_pad);

        local int offset;
        if (lid == BOUND_N_SPLIT_WORK_GROUP_SIZE - 1) {
            cnt = sum;
            start = atomic_add(out_range_cnt, cnt);

            if (cnt + start >= BATCH_SIZE) {
                offset = max(0,in_range_cnt[wid]);
                in_range_cnt[wid] = offset + stack_height + cnt - max(0,(int)BATCH_SIZE - start);
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        // Prefix sum is inclusive, decrement it to get exclusive result
        sum--;
        
        if ((bound_flags & 1) != 0 && start + sum < BATCH_SIZE) {
            out_pids[start + sum] = rpid;
            out_mins[start + sum] = rmin;
            out_maxs[start + sum] = rmax;
        }

        if (cnt + start >= BATCH_SIZE) {
            
            // Output buffer overflow
            // Put local/private patches back in input buffer and exit;
                        
            // Copy stack content back to input buffer
            int wi = 0;
            for (wi = 0; wi+BOUND_N_SPLIT_WORK_GROUP_SIZE < stack_height; wi += BOUND_N_SPLIT_WORK_GROUP_SIZE) {
                size_t pos = lid + wi + offset + wid * in_buffer_stride;
                // pack stack depth back into pid value
                uint x = pid_stack[lid+wi] | (depth_stack[lid+wi]<<24);
                in_pids[pos] = x;
                in_mins[pos] = min_stack[lid + wi];
                in_maxs[pos] = max_stack[lid + wi];
            }
            if (lid + wi < stack_height) {
                size_t pos = lid + wi + offset + wid * in_buffer_stride;

                // pack stack depth back into pid value
                uint x = pid_stack[lid+wi] | (depth_stack[lid+wi]<<24);
                in_pids[pos] = x;
                in_mins[pos] = min_stack[lid + wi];
                in_maxs[pos] = max_stack[lid + wi];
            }

            // Copy bounded, but overflowed ranges back to input buffer
            if ((bound_flags & 1) != 0 && start + sum >= BATCH_SIZE) {
                size_t pos = sum - max(0,(int)BATCH_SIZE - start) + stack_height + offset + wid * in_buffer_stride;

                // pack stack depth back into pid value
                uint x = rpid | (rdepth<<24);
                in_pids[pos] = x;
                in_mins[pos] = rmin;
                in_maxs[pos] = rmax;                
            }

            return;
        }
        

        barrier(CLK_LOCAL_MEM_FENCE);
        
    }
    
}
                  
                  

kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1,1)))
void init_range_buffers(global uint* pids,
                        global float2* mins,
                        global float2* maxs,
                        int patch_count,
                        int buffer_stride)
{
    size_t items_per_work_group = round_up_div(patch_count, BOUND_N_SPLIT_WORK_GROUP_CNT);
    
    size_t gid = get_global_id(0);
    size_t wid = gid % BOUND_N_SPLIT_WORK_GROUP_CNT;
    size_t lid = gid / BOUND_N_SPLIT_WORK_GROUP_CNT;
    
    if (gid < patch_count) {
        size_t pos = lid + wid * buffer_stride;
        pids[pos] = gid;
        mins[pos] = (float2)(0,0);
        maxs[pos] = (float2)(1,1);
    }
}


kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_CNT,1,1)))
void init_count_buffers(global int* in_range_cnt,
                        global int* out_range_cnt,

                        volatile global int* processed_count,
                        
                        int patch_count)
{
    size_t lid = get_global_id(0);
    size_t ilid = BOUND_N_SPLIT_WORK_GROUP_CNT - lid - 1;

    in_range_cnt[lid] = (patch_count+ilid)/BOUND_N_SPLIT_WORK_GROUP_CNT;
    processed_count[lid] = 0;
}
                       
 
