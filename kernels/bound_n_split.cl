
#include "utility.h"


// Compile time constants:
// BATCH_SIZE                    - size_t
// BOUND_N_SPLIT_LIMIT           - float
// BOUND_N_SPLIT_WORK_GROUP_CNT  - int
// BOUND_N_SPLIT_WORK_GROUP_SIZE - int
// BOUND_SAMPLE_RATE             - int
// CULL_RIBBON                   - float
// MAX_SPLIT_DEPTH               - int


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
#define CULL 0
#define DRAW 1
#define HSPLIT 2
#define VSPLIT 6
uchar bound(const global float4* patch_buffer,
           int rpid, float2 rmin, float2 rmax, uchar rdepth,
           private const matrix4* mv, constant const projection* P, float split_limit)
{   
    // Calculate bounding box and max u/v length of patch 
    float2 ppos[RES][RES];
    
    float3 bbox_min = (float3)( INFINITY, INFINITY, INFINITY);
    float3 bbox_max = (float3)(-INFINITY,-INFINITY,-INFINITY);

    uchar mask = (rdepth >= MAX_SPLIT_DEPTH-1) ? 0 : 0xff;
    
    for (size_t u = 0; u < RES; ++u) {
        for (size_t v = 0; v < RES; ++v) {
            float2 uv = mix(rmin, rmax, (float2)(u * (1.0f / (RES-1)), v * (1.0f / (RES-1))));

            float4 p = eval_patch(patch_buffer, rpid, uv);
            p = mul_pm4v4(mv, p);

            bbox_min = min(bbox_min, p.xyz);
            bbox_max = max(bbox_max, p.xyz);
            
            p = mul_cm4v4(&P->proj, p);
            ppos[v][u] = mul_cm2v2(&P->screen_matrix, p.xy / p.w);            
        }
    }

    if (outside_frustum(bbox_min, bbox_max, P)) {
        return CULL;
    } else if (bbox_min.z < 0 && bbox_max.z > 0) {
        return (((rmax.x - rmin.x) < (rmax.y - rmin.y)) ? VSPLIT : HSPLIT) & mask;
    } else {

        float hlen=0, vlen=0;
        for (size_t i = 0; i < RES; ++i) {
            float h = 0, v = 0;
            for (size_t j = 0; j < RES-1; ++j) {
                h += distance(ppos[i][j], ppos[i][j+1]);
                v += distance(ppos[j][i], ppos[j+1][i]);
            }
        
            hlen = max(h, hlen);
            vlen = max(v, vlen);
        }

        if (hlen <= split_limit && vlen <= split_limit) {
            return DRAW;
        } else {
            return ((hlen > vlen) ? VSPLIT : HSPLIT) & mask;
        }
    }

    // unreachable
}




kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1, 1)))
void bound_n_split(const global float4* patch_buffer,

                   int in_buffer_stride,
                   global int* in_pids,
                   global float2* in_mins,
                   global float2* in_maxs,
                   volatile global int* in_range_cnt,
           
                   global int* out_pids,
                   global float2* out_mins,
                   global float2* out_maxs,
                   volatile global int* out_range_cnt,

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
        top = 42;
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
        }
    
        barrier(CLK_LOCAL_MEM_FENCE);

        if (cnt == 0 && stack_cnt == 0) {
            return; // Global exit condition
        }

        if (lid < cnt) {
            size_t pos = wid * in_buffer_stride + start + lid;
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

        // Move bounded ranges to output buffer
        int sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 1) >> 0, prefix_pad);

        if (lid == BOUND_N_SPLIT_WORK_GROUP_SIZE - 1) {

            cnt = sum;
            start = atomic_add(out_range_cnt, cnt) - 1;
            #warning handle overflow
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        if ((bound_flags & 1) != 0 && start + sum < BATCH_SIZE) {
            out_pids[start + sum] = rpid;
            out_mins[start + sum] = rmin;
            out_maxs[start + sum] = rmax;
        }

        // Perform split
        sum = prefix_sum(lid, BOUND_N_SPLIT_WORK_GROUP_SIZE, (bound_flags & 2) >> 1, prefix_pad);

        if (lid == BOUND_N_SPLIT_WORK_GROUP_SIZE - 1) {

            cnt = sum;
            start = stack_height - 2;
            stack_height += cnt * 2;

        }

        barrier(CLK_LOCAL_MEM_FENCE);

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

        barrier(CLK_LOCAL_MEM_FENCE);

    }
    
}
                  
                  

kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_SIZE, 1,1)))
void init_range_buffers(global int* pids,
                        global float2* mins,
                        global float2* maxs,
                        int patch_count,
                        int buffer_stride)
{
    size_t items_per_work_group = round_up_div(patch_count, BOUND_N_SPLIT_WORK_GROUP_CNT);
    
    size_t gid = get_global_id(0);
    size_t wid = gid / items_per_work_group;
    size_t lid = gid % items_per_work_group;
    
    if (gid < patch_count) {
        size_t pos = lid + wid * buffer_stride;
        pids[pos] = gid;
        mins[pos] = (float2)(0,0);
        maxs[pos] = (float2)(1,1);
    }
}


kernel __attribute__((reqd_work_group_size(1,1,1)))
void init_projection_buffer(global projection* P,
                       
                            matrix4 proj,
                            matrix2 screen_matrix,
                            float fovy,
                            float2 f,
                            float near,
                            float far,
                            int2 screen_size)
{
    P->proj = proj;
    P->screen_matrix = screen_matrix;
    P->fovy = fovy;
    P->f = f;
    P->near = near;
    P->far = far;
    P->screen_size = screen_size;
}


kernel __attribute__((reqd_work_group_size(BOUND_N_SPLIT_WORK_GROUP_CNT,1,1)))
void init_count_buffers(global int* in_range_cnt,
                        global int* out_range_cnt,

                       int patch_count)
{
    size_t lid = get_global_id(0);
    size_t ilid = BOUND_N_SPLIT_WORK_GROUP_SIZE - lid - 1;
    in_range_cnt[lid] = (patch_count+ilid)/BOUND_N_SPLIT_WORK_GROUP_CNT;

    if (get_global_id(0) == 0) {
        *out_range_cnt = 0;
    }
}
                       
 
