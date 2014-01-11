#include "BoundNSplitCL.h"


#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"


struct cl_projection
{
    mat4 proj;
    mat2 screen_matrix;
    float fovy;
    vec2 f;
    float near, far;
    ivec2 screen_size;
};


#define BATCH_SIZE config.reyes_patches_per_pass()
#define MAX_SPLIT_DEPTH config.max_split_depth()

Reyes::BoundNSplitCLMultipass::BoundNSplitCLMultipass(CL::Device& device,
                                                      CL::CommandQueue& queue,
                                                      shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)
      
    , _out_pids_buffer(device, BATCH_SIZE * sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_mins_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_maxs_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_range_cnt_buffer(device, sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY)

    , _projection_buffer(device, sizeof(cl_projection), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)

    , _prefix_sum(device, BATCH_SIZE)
{
    _patch_index->enable_load_opencl_buffer(device, queue);

    
    _bound_n_split_program.set_constant("BOUND_SAMPLE_RATE", config.bound_sample_rate());
    _bound_n_split_program.set_constant("CULL_RIBBON", config.cull_ribbon());
    _bound_n_split_program.set_constant("MAX_SPLIT_DEPTH", config.max_split_depth());

    _bound_n_split_program.compile(device, "bound_n_split_multipass.cl");

    _bound_kernel.reset(_bound_n_split_program.get_kernel("bound_kernel"));
}


void Reyes::BoundNSplitCLMultipass::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;
}


bool Reyes::BoundNSplitCLMultipass::done()
{
    return true;
}


void Reyes::BoundNSplitCLMultipass::finish()
{
    
}


Reyes::Batch Reyes::BoundNSplitCLMultipass::do_bound_n_split(CL::Event& ready)
{
    return {(size_t)0, *_active_patch_buffer, _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _ready};
}
