#include "OpenCLBoundNSplit.h"


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
Reyes::OpenCLBoundNSplitLocal::OpenCLBoundNSplitLocal(CL::Device& device,
                                                      CL::CommandQueue& queue,
                                                      shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)

    , _active_handle(nullptr)
    , _active_patch_buffer(nullptr)
      
    , _in_pids_buffer(device, BATCH_SIZE * sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _in_mins_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _in_maxs_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _in_range_cnt_buffer(device, sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
      
    , _out_pids_buffer(device, BATCH_SIZE * sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_mins_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_maxs_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_range_cnt_buffer(device, sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY)

    , _projection_buffer(device, queue, sizeof(cl_projection), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
{
    _patch_index->enable_load_opencl_buffer(device, queue);
    
    _bound_n_split_program.set_constant("CULL_RIBBON", config.cull_ribbon());
    _bound_n_split_program.set_constant("BOUND_N_SPLIT_WORK_GROUP_SIZE", 64);
    _bound_n_split_program.set_constant("BOUND_SAMPLE_RATE", config.bound_sample_rate());
    _bound_n_split_program.set_constant("MAX_SPLIT_DEPTH", config.max_split_depth());
    _bound_n_split_program.set_constant("BATCH_SIZE", config.reyes_patches_per_pass());
    
    _bound_n_split_program.compile(device, "bound_n_split.cl");
    
    _bound_n_split_kernel.reset(_bound_n_split_program.get_kernel("bound_n_split"));
    _init_range_buffers_kernel.reset(_bound_n_split_program.get_kernel("init_range_buffers"));
    _init_flag_buffers_kernel.reset(_bound_n_split_program.get_kernel("init_flag_buffers"));
}


void Reyes::OpenCLBoundNSplitLocal::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;
    
    int patch_count = _patch_index->get_patch_count(patches_handle);

    
    {
        // TODO: Redo this only when projection has changed

        mat4 proj;
        mat2 screen_matrix;

        projection->calc_projection(proj);
        projection->calc_screen_matrix(screen_matrix);
        
        _init_flag_buffers_kernel->set_args(_projection_buffer, _in_range_cnt_buffer, _out_range_cnt_buffer,
                                            proj, screen_matrix, projection->fovy(), projection->f(),
                                            projection->near(), projection->far(), projection->viewport_i(),
                                            patch_count);
        _ready = _queue.enq_kernel(*_init_flag_buffers_kernel, 1,1, "initialize flag buffers", _ready);
                                            
    }
    
    
    
    _init_range_buffers_kernel->set_args(_in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, patch_count);
    _ready = _queue.enq_kernel(*_init_range_buffers_kernel,
                               (int)ceil(patch_count / 64.0) * 64, 64,
                               "initialize range buffers", _ready);

    
    //_queue.flush();
}


bool Reyes::OpenCLBoundNSplitLocal::done()
{
    if (_active_handle == nullptr) return true;

    return false;
}


void Reyes::OpenCLBoundNSplitLocal::finish()
{
    _queue.wait_for_events(_ready);
    _ready = CL::Event();
}


Reyes::Batch Reyes::OpenCLBoundNSplitLocal::do_bound_n_split(CL::Event& ready)
{
    size_t patch_count = _patch_index->get_patch_count(_active_handle);
    
    _bound_n_split_kernel->set_args(*_active_patch_buffer,
                                    _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                                    _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _out_range_cnt_buffer,
                                    _active_matrix, _projection_buffer,
                                    config.bound_n_split_limit());
    _ready = _queue.enq_kernel(*_bound_n_split_kernel, 64 * 32, 64, "bound & split", _ready | ready);

    int out_range_cnt;

    _ready = _queue.enq_read_buffer(_out_range_cnt_buffer, &out_range_cnt, sizeof(out_range_cnt), "read patch count", _ready);
    
    _queue.flush();
    _queue.wait_for_events(_ready);
    _ready = CL::Event();
    
    _active_handle = nullptr;

    // TODO: Handle this properly
    out_range_cnt = std::min((int)BATCH_SIZE, out_range_cnt);
    
    return {(size_t)out_range_cnt, *_active_patch_buffer, _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _ready};
}
