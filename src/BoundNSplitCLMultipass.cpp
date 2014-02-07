#include "BoundNSplitCLMultipass.h"

#include "CL/PrefixSum.h"
#include "Config.h"
#include "PatchIndex.h"
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
#define PROCESS_CNT BATCH_SIZE
#define MAX_SPLIT_DEPTH config.max_split_depth()

Reyes::BoundNSplitCLMultipass::BoundNSplitCLMultipass(CL::Device& device,
                                                      CL::CommandQueue& queue,
                                                      shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)

    , _pid_stack(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _depth_stack(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _min_stack(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _max_stack(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)

    , _split_ranges_cnt_buffer(device, sizeof(cl_int), CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY)
      
    , _out_pids_buffer(device, BATCH_SIZE * sizeof(cl_int) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_mins_buffer(device, BATCH_SIZE * sizeof(cl_float2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_maxs_buffer(device, BATCH_SIZE * sizeof(cl_float2) , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _out_range_cnt_buffer(device, sizeof(cl_int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY)

    , _bound_flags(device, BATCH_SIZE * sizeof(cl_uchar), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _draw_flags(device, BATCH_SIZE * sizeof(cl_int), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _split_flags(device, BATCH_SIZE * sizeof(cl_int), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)

    , _pid_pad(device, BATCH_SIZE * sizeof(cl_int), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _depth_pad(device, BATCH_SIZE * sizeof(cl_uchar), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _min_pad(device, BATCH_SIZE * sizeof(cl_float2), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    , _max_pad(device, BATCH_SIZE * sizeof(cl_float2), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS)
    
    , _projection_buffer(device, sizeof(cl_projection), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)

    , _prefix_sum(device, BATCH_SIZE)
{
    _patch_index->enable_load_opencl_buffer(device, queue);

    
    _bound_n_split_program.set_constant("BOUND_SAMPLE_RATE", config.bound_sample_rate());
    _bound_n_split_program.set_constant("CULL_RIBBON", config.cull_ribbon());
    _bound_n_split_program.set_constant("MAX_SPLIT_DEPTH", config.max_split_depth());

    _bound_n_split_program.compile(device, "bound_n_split_multipass.cl");

    _bound_kernel.reset(_bound_n_split_program.get_kernel("bound_kernel"));
    _move_kernel.reset(_bound_n_split_program.get_kernel("move"));
    _init_ranges_kernel.reset(_bound_n_split_program.get_kernel("init_ranges"));
    _init_projection_buffer_kernel.reset(_bound_n_split_program.get_kernel("init_projection_buffer"));

    _ready = CL::Event();

    _ready = _queue.enq_fill_buffer(_bound_flags, (cl_uchar)0, BATCH_SIZE, "Init bound flags buffer", _ready);
    _ready = _queue.enq_fill_buffer(_draw_flags, (cl_int)0, BATCH_SIZE, "Init bound flags buffer", _ready);
    _ready = _queue.enq_fill_buffer(_split_flags, (cl_int)0, BATCH_SIZE, "Init bound flags buffer", _ready);
}


void Reyes::BoundNSplitCLMultipass::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;

    size_t patch_count = _patch_index->get_patch_count(patches_handle);

    size_t stack_size = patch_count + PROCESS_CNT * MAX_SPLIT_DEPTH;
    
    _stack_height = patch_count;

    if (_depth_stack.get_size() < stack_size) {
        _pid_stack.resize(stack_size * sizeof(cl_int));
        _depth_stack.resize(stack_size * sizeof(cl_uchar));
        _min_stack.resize(stack_size * sizeof(cl_float2));
        _max_stack.resize(stack_size * sizeof(cl_float2));
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

    //_queue.finish();
    _init_ranges_kernel->set_args((cl_int)patch_count, _pid_stack, _depth_stack, _min_stack, _max_stack);
    _ready = _queue.enq_kernel(*_init_ranges_kernel, round_up_by((int)patch_count, 64), 64, "init patch ranges", _ready);
}


bool Reyes::BoundNSplitCLMultipass::done()
{   


    return (_stack_height <= 0);
}


void Reyes::BoundNSplitCLMultipass::finish()
{
    _queue.wait_for_events(_ready);
    _ready = CL::Event();    
}


Reyes::Batch Reyes::BoundNSplitCLMultipass::do_bound_n_split(CL::Event& ready)
{
    CL::Event prefix_sum_ready, mapping_ready;
    
    int batch_size = std::min((int)_stack_height, (int)BATCH_SIZE);
    
    _stack_height -= batch_size;
    
    _bound_kernel->set_args(*_active_patch_buffer,
                            batch_size, _stack_height,
                            _pid_stack, _depth_stack, _min_stack, _max_stack,
                            _bound_flags, _split_flags, _draw_flags,
                            _pid_pad, _depth_pad, _min_pad, _max_pad,
                            _active_matrix, _projection_buffer, config.bound_n_split_limit());
    _ready = _queue.enq_kernel(*_bound_kernel, round_up_by(batch_size, 64), 64, "bound patches", ready | _ready);

    prefix_sum_ready = _prefix_sum.apply(batch_size, _queue,
                                         _split_flags, _split_flags, _split_ranges_cnt_buffer,
                                         _ready);
    prefix_sum_ready = _prefix_sum.apply(batch_size, _queue,
                                         _draw_flags, _draw_flags, _out_range_cnt_buffer,
                                         prefix_sum_ready);

    
    _move_kernel->set_args(batch_size, _stack_height,
                           _pid_pad, _depth_pad, _min_pad, _max_pad,
                           _bound_flags, _draw_flags, _split_flags,
                           _pid_stack, _depth_stack, _min_stack, _max_stack,
                           _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer);
    _ready = _queue.enq_kernel(*_move_kernel, round_up_by(batch_size, 64), 64, "split patches", _ready | prefix_sum_ready);

    mapping_ready = _queue.enq_map_buffer(_out_range_cnt_buffer, CL_MAP_READ, "buffer map", prefix_sum_ready);
    mapping_ready = _queue.enq_map_buffer(_split_ranges_cnt_buffer, CL_MAP_READ, "buffer map", mapping_ready);

    _queue.wait_for_events(mapping_ready);
    
    int draw_count = _out_range_cnt_buffer.host_ref<int>();
    int split_count = _split_ranges_cnt_buffer.host_ref<int>(); 
    _stack_height += split_count * 2;

    _queue.enq_unmap_buffer(_out_range_cnt_buffer, "buffer map", CL::Event());
    _queue.enq_unmap_buffer(_split_ranges_cnt_buffer, "buffer map", CL::Event());
    
    //cout << split_count << "split, " << draw_count << "drawn" << endl;

    statistics.add_patches(draw_count);
    statistics.inc_pass_count(1);
    
    return {config.dummy_render() ? 0 : (size_t)draw_count, *_active_patch_buffer, _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer, _ready};
}
