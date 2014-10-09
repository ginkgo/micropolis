#include "BoundNSplitCLLocal.h"


#include "PatchIndex.h"
#include "ReyesConfig.h"
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


#define BATCH_SIZE reyes_config.reyes_patches_per_pass()
#define WORK_GROUP_CNT reyes_config.local_bns_work_groups()
#define WORK_GROUP_SIZE (_queue.device().preferred_work_group_size_multiple())
#define MAX_SPLIT_DEPTH reyes_config.max_split_depth()
#define MAX_BNS_ITERATIONS 200

Reyes::BoundNSplitCLLocal::BoundNSplitCLLocal(CL::Device& device,
                                              CL::CommandQueue& queue,
                                              shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)

    , _active_handle(nullptr)
    , _active_patch_buffer(nullptr)

    , _in_buffers_size(0)
    , _in_buffer_stride(0)
    , _in_pids_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_mins_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_maxs_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _in_range_cnt_buffer(device, WORK_GROUP_CNT * sizeof(int), CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")
      
    , _out_pids_buffer(device, BATCH_SIZE * sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_mins_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_maxs_buffer(device, BATCH_SIZE * sizeof(vec2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_range_cnt_buffer(device, sizeof(int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")

    , _processed_count_buffer(device, WORK_GROUP_CNT * sizeof(int), CL_MEM_READ_WRITE, "bound&split")

    , _projection_buffer(device, sizeof(cl_projection), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, "bound&split")
{
    _patch_index->enable_load_opencl_buffer(device, queue);

    for (auto program : {&_bound_n_split_program_bezier, &_bound_n_split_program_gregory}) {
        program->set_constant("BATCH_SIZE", reyes_config.reyes_patches_per_pass());
        program->set_constant("BOUND_N_SPLIT_WORK_GROUP_CNT", WORK_GROUP_CNT);
        program->set_constant("BOUND_N_SPLIT_WORK_GROUP_SIZE", WORK_GROUP_SIZE);
        program->set_constant("BOUND_SAMPLE_RATE", reyes_config.bound_sample_rate());
        program->set_constant("CULL_RIBBON", reyes_config.cull_ribbon());
        program->set_constant("MAX_SPLIT_DEPTH", reyes_config.max_split_depth());
    }


    _bound_n_split_program_bezier.define("eval_patch", "eval_bezier_patch");
    _bound_n_split_program_bezier.compile(device, "bound_n_split_local.cl");
    
    _bound_n_split_program_gregory.define("eval_patch", "eval_gregory_patch");
    _bound_n_split_program_gregory.compile(device, "bound_n_split_local.cl");
    
    _bound_n_split_kernel_bezier.reset(_bound_n_split_program_bezier.get_kernel("bound_n_split"));
    _bound_n_split_kernel_gregory.reset(_bound_n_split_program_gregory.get_kernel("bound_n_split"));
    
    _init_range_buffers_kernel.reset(_bound_n_split_program_bezier.get_kernel("init_range_buffers"));
    _init_projection_buffer_kernel.reset(_bound_n_split_program_bezier.get_kernel("init_projection_buffer"));
    _init_count_buffers_kernel.reset(_bound_n_split_program_bezier.get_kernel("init_count_buffers"));
}


void Reyes::BoundNSplitCLLocal::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;
    _active_patch_type = _patch_index->get_patch_type(patches_handle);
    
    size_t patch_count = _patch_index->get_patch_count(patches_handle);

    if (_in_buffers_size < patch_count) {
        _in_buffers_size = patch_count;

        size_t item_count = round_up_by(_in_buffers_size, WORK_GROUP_CNT) + MAX_SPLIT_DEPTH * WORK_GROUP_SIZE * WORK_GROUP_CNT;

        assert(item_count % WORK_GROUP_CNT == 0);        
        _in_buffer_stride = item_count / WORK_GROUP_CNT;
        
        _in_pids_buffer.resize(item_count * sizeof(int));
        _in_mins_buffer.resize(item_count * sizeof(vec2));
        _in_maxs_buffer.resize(item_count * sizeof(vec2));
    }
    
    
    {
        // TODO: Resend this only when projection has changed
        mat4 proj;
        mat2 screen_matrix;

        projection->calc_projection(proj);
        projection->calc_screen_matrix(screen_matrix);
        
        _init_projection_buffer_kernel->set_args(_projection_buffer, 
                                                 proj, screen_matrix, projection->fovy(), projection->f(),
                                                 projection->near(), projection->far(), projection->viewport_i());
        _ready = _queue.enq_kernel(*_init_projection_buffer_kernel, 1,1, "initialize projection buffer", _ready);
    }

    _init_count_buffers_kernel->set_args(_in_range_cnt_buffer, _out_range_cnt_buffer,
                                         _processed_count_buffer,
                                         (cl_int)patch_count);
    _ready = _queue.enq_kernel(*_init_count_buffers_kernel, WORK_GROUP_CNT, WORK_GROUP_CNT,
                               "initialize counter buffers", _ready);
    
    
    _init_range_buffers_kernel->set_args(_in_pids_buffer, _in_mins_buffer, _in_maxs_buffer,
                                         (cl_int)patch_count, (cl_int)_in_buffer_stride);
    _ready = _queue.enq_kernel(*_init_range_buffers_kernel,
                               (int)round_up_by(patch_count, WORK_GROUP_SIZE), WORK_GROUP_SIZE,
                               "initialize range buffers", _ready);
    
    //_queue.flush();
    _done = false;
    _iteration_count = 0;
}


bool Reyes::BoundNSplitCLLocal::done()
{
    if (_done) {

        CL::Event ready = _queue.enq_read_buffer(_processed_count_buffer,
                                                 _processed_count_buffer.void_ptr(),
                                                 _processed_count_buffer.get_size(),
                                                 "read processed counts", CL::Event());
        _queue.flush();
        _queue.wait_for_events(ready);

        cl_int* processed = _processed_count_buffer.host_ptr<cl_int>();
        statistics.set_bound_n_split_balance(processed, WORK_GROUP_CNT);

        for (size_t i = 0; i < WORK_GROUP_CNT; ++i) {
            statistics.add_bounds(processed[i]);
        }
    }
    
    return _done;
}


void Reyes::BoundNSplitCLLocal::finish()
{
    _queue.wait_for_events(_ready);
    _ready = CL::Event();

    // if (reyes_config.debug_work_group_balance()) {
        
    //     CL::Event ready = _queue.enq_read_buffer(_processed_count_buffer,
    //                                              _processed_count_buffer.void_ptr(),
    //                                              _processed_count_buffer.get_size(),
    //                                              "read processed counts", CL::Event());
    //     _queue.flush();
    //     _queue.wait_for_events(ready);

    //     cl_int* processed = _processed_count_buffer.host_ptr<cl_int>();
    //     statistics.set_bound_n_split_balance(processed, WORK_GROUP_CNT);

    //     size_t total_processed = 0;

    //     for (size_t i = 0; i < WORK_GROUP_CNT; ++i) {
    //         total_processed += processed[i];
    //     }

    //     statistics.add_bounds(total_processed);
    // }
}


Reyes::Batch Reyes::BoundNSplitCLLocal::do_bound_n_split(CL::Event& ready)
{
    size_t patch_count = _patch_index->get_patch_count(_active_handle);

    _ready = _queue.enq_fill_buffer(_out_range_cnt_buffer, (cl_int)0, 1, "Clear out_range_cnt", _ready | ready);

    switch(_active_patch_type) {
    case Reyes::BEZIER:
        _bound_n_split_kernel_bezier->set_args(*_active_patch_buffer,
                                               (cl_int)_in_buffer_stride,
                                               _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                                               _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _out_range_cnt_buffer,
                                               _processed_count_buffer,
                                               _active_matrix, _projection_buffer,
                                               reyes_config.bound_n_split_limit());
        _ready = _queue.enq_kernel(*_bound_n_split_kernel_bezier,
                                   ivec2(WORK_GROUP_SIZE,  WORK_GROUP_CNT), ivec2(WORK_GROUP_SIZE, 1),
                                   "bound & split", _ready);
        break;
    case Reyes::GREGORY:
        _bound_n_split_kernel_gregory->set_args(*_active_patch_buffer,
                                                (cl_int)_in_buffer_stride,
                                                _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                                                _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _out_range_cnt_buffer,
                                                _processed_count_buffer,
                                                _active_matrix, _projection_buffer,
                                                reyes_config.bound_n_split_limit());
        _ready = _queue.enq_kernel(*_bound_n_split_kernel_gregory,
                                   ivec2(WORK_GROUP_SIZE,  WORK_GROUP_CNT), ivec2(WORK_GROUP_SIZE, 1),
                                   "bound & split", _ready);
        break;
    }

    _ready = _queue.enq_read_buffer(_out_range_cnt_buffer, _out_range_cnt_buffer.void_ptr(), _out_range_cnt_buffer.get_size(),
                                    "read range count", _ready);
    _queue.flush();
    _queue.wait_for_events(_ready);
    
    int out_range_cnt = _out_range_cnt_buffer.host_ref<cl_int>();

    _ready = CL::Event();
    _active_handle = nullptr;

    statistics.add_patches(out_range_cnt);
    
    out_range_cnt = std::min((int)BATCH_SIZE, out_range_cnt);
    
    
    _iteration_count++;
    
    if (out_range_cnt <= 0 || _iteration_count > MAX_BNS_ITERATIONS) {
        _done = true;
    } else {
        statistics.inc_pass_count(1);
    }
    
    return {reyes_config.dummy_render() ? 0 : (size_t)out_range_cnt,
            _active_patch_type, *_active_patch_buffer,
            _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer,
            _ready};
}
