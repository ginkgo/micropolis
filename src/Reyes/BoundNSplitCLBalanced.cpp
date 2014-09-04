#include "BoundNSplitCLBalanced.h"


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
#define WORK_GROUP_CNT config.local_bns_work_groups()
#define WORK_GROUP_SIZE (_queue.device().preferred_work_group_size_multiple())
#define MAX_SPLIT_DEPTH config.max_split_depth()
#define MAX_BNS_ITERATIONS 200

Reyes::BoundNSplitCLBalanced::BoundNSplitCLBalanced(CL::Device& device,
                                                    CL::CommandQueue& queue,
                                                    shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)

    , _active_handle(nullptr)
    , _active_patch_buffer(nullptr)

    , _in_buffers_size(0)
    , _in_pids_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_mins_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_maxs_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_range_cnt_buffer(device, sizeof(int), CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")
      
    , _out_pids_buffer(device, BATCH_SIZE * sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_mins_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_maxs_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_range_cnt_buffer(device, sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")

    , _processed_count_buffer(device, WORK_GROUP_CNT * sizeof(int), CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")

    , _projection_buffer(device, sizeof(cl_projection), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
{
    _patch_index->enable_load_opencl_buffer(device, queue);
    
    _bound_n_split_program.set_constant("BATCH_SIZE", config.reyes_patches_per_pass());
    _bound_n_split_program.set_constant("BOUND_N_SPLIT_WORK_GROUP_CNT", WORK_GROUP_CNT);
    _bound_n_split_program.set_constant("BOUND_N_SPLIT_WORK_GROUP_SIZE", WORK_GROUP_SIZE);
    _bound_n_split_program.set_constant("BOUND_SAMPLE_RATE", config.bound_sample_rate());
    _bound_n_split_program.set_constant("CULL_RIBBON", config.cull_ribbon());
    _bound_n_split_program.set_constant("MAX_SPLIT_DEPTH", config.max_split_depth());
    
    _bound_n_split_program.compile(device, "bound_n_split_balanced.cl");
    
    _bound_n_split_kernel.reset(_bound_n_split_program.get_kernel("bound_n_split"));
    _init_range_buffers_kernel.reset(_bound_n_split_program.get_kernel("init_range_buffers"));
    _init_projection_buffer_kernel.reset(_bound_n_split_program.get_kernel("init_projection_buffer"));
}


void Reyes::BoundNSplitCLBalanced::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;
    
    size_t patch_count = _patch_index->get_patch_count(patches_handle);

    if (_in_buffers_size < patch_count) {
        _in_buffers_size = patch_count;

        size_t item_count = patch_count;
        
        _in_pids_buffer.resize(item_count * sizeof(int));
        _in_mins_buffer.resize(item_count * sizeof(vec2));
        _in_maxs_buffer.resize(item_count * sizeof(vec2));
    }
    
    
    {
        // TODO: Redo this only when projection has changed
        mat4 proj;
        mat2 screen_matrix;

        projection->calc_projection(proj);
        projection->calc_screen_matrix(screen_matrix);
        
        _init_projection_buffer_kernel->set_args(_projection_buffer, 
                                                 proj, screen_matrix, projection->fovy(), projection->f(),
                                                 projection->near(), projection->far(), projection->viewport_i());
        _ready = _queue.enq_kernel(*_init_projection_buffer_kernel, 1,1, "initialize projection buffer", _ready);
    }

    _ready = _queue.enq_fill_buffer(_in_range_cnt_buffer, (cl_int)patch_count, 1, "init buffer counter", _ready);
    
    _init_range_buffers_kernel->set_args(_in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, (cl_int)patch_count);
    _ready = _queue.enq_kernel(*_init_range_buffers_kernel,
                               (int)round_up_by(patch_count, WORK_GROUP_SIZE), WORK_GROUP_SIZE,
                               "initialize range buffers", _ready);

    _ready = _queue.enq_fill_buffer(_processed_count_buffer, (cl_int)0, WORK_GROUP_CNT, "clear processed count buffer", _ready);
    
    //_queue.flush();
    _done = false;
    _iteration_count = 0;
}


bool Reyes::BoundNSplitCLBalanced::done()
{
    return _done;
}


void Reyes::BoundNSplitCLBalanced::finish()
{
    _queue.wait_for_events(_ready);
    _ready = CL::Event();

    if (config.debug_work_group_balance()) {
        
        CL::Event ready = _queue.enq_read_buffer(_processed_count_buffer,
                                                 _processed_count_buffer.void_ptr(),
                                                 _processed_count_buffer.get_size(),
                                                 "read processed counts", CL::Event());
        _queue.flush();
        _queue.wait_for_events(ready);

        cl_int* processed = _processed_count_buffer.host_ptr<cl_int>();

        statistics.set_bound_n_split_balance(processed, WORK_GROUP_CNT);
        // for (int i = 0; i < WORK_GROUP_CNT; ++i) {
        //     cout << processed[i] << " ";
        // }
        // cout << endl;
    }
}


Reyes::Batch Reyes::BoundNSplitCLBalanced::do_bound_n_split(CL::Event& ready)
{
    size_t patch_count = _patch_index->get_patch_count(_active_handle);

    _ready = _queue.enq_fill_buffer(_out_range_cnt_buffer, (cl_int)0, 1, "Clear out_range_cnt", _ready | ready);
    
    _bound_n_split_kernel->set_args(*_active_patch_buffer,
                                    _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                                    _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _out_range_cnt_buffer,
                                    _processed_count_buffer,
                                    _active_matrix, _projection_buffer,
                                    config.bound_n_split_limit());
    _ready = _queue.enq_kernel(*_bound_n_split_kernel,
                               ivec2(WORK_GROUP_SIZE,  WORK_GROUP_CNT), ivec2(WORK_GROUP_SIZE, 1),
                               "bound & split", _ready);

    _ready = _queue.enq_read_buffer(_out_range_cnt_buffer, _out_range_cnt_buffer.void_ptr(), _out_range_cnt_buffer.get_size(),
                                    "read range count", _ready);
    _queue.flush();
    _queue.wait_for_events(_ready);
    
    int out_range_cnt = _out_range_cnt_buffer.host_ref<cl_int>();

    _ready = CL::Event();
    _active_handle = nullptr;

    out_range_cnt = std::min((int)BATCH_SIZE, out_range_cnt);

    _iteration_count++;
    
    if (out_range_cnt <= 0 || _iteration_count > MAX_BNS_ITERATIONS) {
        _done = true;
    } else {
        statistics.inc_pass_count(1);
        statistics.add_patches(out_range_cnt);
    }
    
    return {config.dummy_render() ? 0 : (size_t)out_range_cnt, *_active_patch_buffer, _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _ready};
}
