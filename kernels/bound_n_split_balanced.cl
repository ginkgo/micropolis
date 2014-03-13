
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

                   global int* in_pids,
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
                
                top = atomic_sub(in_range_cnt, cnt);

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
            size_t pos = start + lid;
            rpid   = in_pids[pos];
            rdepth = 0;
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

            int pos0 = start + sum * 2;
            int pos1 = pos0 + 1;
            
            depth_stack[pos0] = rdepth+1;
            pid_stack  [pos0] = rpid;
            
            depth_stack[pos1] = rdepth+1;
            pid_stack  [pos1] = rpid;
            
            float2 c = (rmin+rmax)*0.5f;

            min_stack[pos0] = (float2)(rmin.x, rmin.y);
            max_stack[pos1] = (float2)(rmax.x, rmax.y);
                
            // Check split direction
            if (bound_flags & 4) {
                // Vertical
                max_stack[pos0] = (float2)(c.x, rmax.y);
                min_stack[pos1] = (float2)(c.x, rmin.y);
            } else {
                // Horizontal
                max_stack[pos0] = (float2)(rmax.x, c.y);
                min_stack[pos1] = (float2)(rmin.x, c.y);
            }
        }

        // Move bounded ranges to output buffer
        sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 1) >> 0, prefix_pad);

        if (lid == BOUND_N_SPLIT_WORK_GROUP_SIZE - 1) {
            cnt = sum;
            start = atomic_add(out_range_cnt, cnt);
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
            // Discard stack and exit

            return;
        }        

        barrier(CLK_LOCAL_MEM_FENCE);
        
    }
    
}
                  
                  

kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1,1)))
void init_range_buffers(global int* pids,
                        global float2* mins,
                        global float2* maxs,
                        int patch_count)
{
    size_t items_per_work_group = round_up_div(patch_count, BOUND_N_SPLIT_WORK_GROUP_CNT);
    
    size_t lid = get_global_id(0);
    
    if (lid < patch_count) {
        pids[lid] = lid;
        mins[lid] = (float2)(0,0);
        maxs[lid] = (float2)(1,1);
    }
}
                       
 
