#include "BoundNSplitCLBreadthFirst.h"

#include "CL/PrefixSum.h"
#include "ReyesConfig.h"
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


#define BATCH_SIZE reyes_config.reyes_patches_per_pass()




Reyes::BoundNSplitCLBreadthFirst::PatchBuffer::PatchBuffer(CL::Device& device)
    : pids(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , depths(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , mins(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , maxs(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , size(0)
{
}


void Reyes::BoundNSplitCLBreadthFirst::PatchBuffer::grow_to(size_t new_size)
{
    if (size >= new_size) return;

    size = new_size;

    cerr << "Resizing to " << new_size << "patches" << endl;
    
    pids.resize(size * sizeof(cl_int));
    depths.resize(size * sizeof(cl_uchar));
    mins.resize(size * sizeof(cl_float2));
    maxs.resize(size * sizeof(cl_float2));    
}




Reyes::BoundNSplitCLBreadthFirst::FlagBuffer::FlagBuffer(CL::Device& device)
    : bound_flags(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , draw_flags(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , split_flags(device, 0, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , size(0)
{
}


void Reyes::BoundNSplitCLBreadthFirst::FlagBuffer::grow_to(size_t new_size)
{
    if (size >= new_size) return;

    size = new_size;
    
    bound_flags.resize(size * sizeof(cl_uchar));
    draw_flags.resize(size * sizeof(cl_int));
    split_flags.resize(size * sizeof(cl_int));
}




Reyes::BoundNSplitCLBreadthFirst::BoundNSplitCLBreadthFirst(CL::Device& device,
                                                            CL::CommandQueue& queue,
                                                            shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)

    , _read_buffers(new PatchBuffer(device))
    , _write_buffers(new PatchBuffer(device))

    , _flag_buffers(device)
    , _prefix_sum(device, 0, "bound&split")
    , _split_ranges_cnt_buffer(device, sizeof(cl_int), CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")

    , _out_pids_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_mins_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_maxs_buffer(device, 0 , CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, "bound&split")
    , _out_range_cnt_buffer(device, sizeof(cl_int) , CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, "bound&split")

    , _projection_buffer(device, sizeof(cl_projection), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, "bound&split")

    , _user_event(device, "bound&split")
{
    _patch_index->enable_load_opencl_buffer(device, queue);


    
    _bound_n_split_program_bezier.set_constant("BOUND_SAMPLE_RATE", reyes_config.bound_sample_rate());
    _bound_n_split_program_bezier.set_constant("CULL_RIBBON", reyes_config.cull_ribbon());
    _bound_n_split_program_bezier.set_constant("MAX_SPLIT_DEPTH", reyes_config.max_split_depth());

    _bound_n_split_program_bezier.define("eval_patch", "eval_bezier_patch");
    _bound_n_split_program_bezier.compile(device, "bound_n_split_breadthfirst.cl");
    
    _bound_n_split_program_gregory.set_constant("BOUND_SAMPLE_RATE", reyes_config.bound_sample_rate());
    _bound_n_split_program_gregory.set_constant("CULL_RIBBON", reyes_config.cull_ribbon());
    _bound_n_split_program_gregory.set_constant("MAX_SPLIT_DEPTH", reyes_config.max_split_depth());

    _bound_n_split_program_gregory.define("eval_patch", "eval_gregory_patch");
    _bound_n_split_program_gregory.compile(device, "bound_n_split_breadthfirst.cl");

    _bound_kernel_bezier.reset(_bound_n_split_program_bezier.get_kernel("bound_kernel"));
    _bound_kernel_gregory.reset(_bound_n_split_program_gregory.get_kernel("bound_kernel"));
    _move_kernel.reset(_bound_n_split_program_bezier.get_kernel("move"));
    _init_ranges_kernel.reset(_bound_n_split_program_bezier.get_kernel("init_ranges"));
    _init_projection_buffer_kernel.reset(_bound_n_split_program_bezier.get_kernel("init_projection_buffer"));
    

    _ready = CL::Event();
}


void Reyes::BoundNSplitCLBreadthFirst::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    _active_matrix = matrix;
    _active_patch_type = _patch_index->get_patch_type(patches_handle);

    _patch_count = _patch_index->get_patch_count(patches_handle);
    
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

    _read_buffers->grow_to(_patch_count);
    
    _init_ranges_kernel->set_args((cl_int)_patch_count,
                                  _read_buffers->pids, _read_buffers->depths, _read_buffers->mins, _read_buffers->maxs);
    _ready = _queue.enq_kernel(*_init_ranges_kernel, round_up_by((int)_patch_count, 64), 64, "init patch ranges", _ready);
}


bool Reyes::BoundNSplitCLBreadthFirst::done()
{   
    return _patch_count <= 0;
}


void Reyes::BoundNSplitCLBreadthFirst::finish()
{
    _queue.wait_for_events(_ready);
    _ready = CL::Event();    
}


Reyes::Batch Reyes::BoundNSplitCLBreadthFirst::do_bound_n_split(CL::Event& ready)
{
    CL::Event prefix_sum_ready, mapping_ready;

    statistics.update_max_patches(_patch_count);
    
    // _queue.wait_for_events(_ready);
    // _ready = CL::Event();

    // Resize buffers on demand
    _write_buffers->grow_to(_patch_count*2);
    _flag_buffers.grow_to(_patch_count);
    _prefix_sum.resize(_patch_count);

    if (_out_pids_buffer.get_size() < _patch_count * sizeof(cl_int)) {
        _queue.wait_for_events(ready);
        ready = CL::Event();

        _out_pids_buffer.resize(_patch_count * sizeof(cl_int));
        _out_mins_buffer.resize(_patch_count * sizeof(cl_float2));
        _out_maxs_buffer.resize(_patch_count * sizeof(cl_float2));
    }

    switch(_active_patch_type) {
    case Reyes::BEZIER:
        _bound_kernel_bezier->set_args(*_active_patch_buffer, _patch_count,
                                       _read_buffers->pids, _read_buffers->depths, _read_buffers->mins, _read_buffers->maxs,
                                       _flag_buffers.bound_flags, _flag_buffers.split_flags, _flag_buffers.draw_flags,
                                       _active_matrix, _projection_buffer, reyes_config.bound_n_split_limit());
        _ready = _queue.enq_kernel(*_bound_kernel_bezier, round_up_by(_patch_count, 64), 64, "bound patches", ready | _ready);
        break;
    case Reyes::GREGORY:
        _bound_kernel_gregory->set_args(*_active_patch_buffer, _patch_count,
                                       _read_buffers->pids, _read_buffers->depths, _read_buffers->mins, _read_buffers->maxs,
                                       _flag_buffers.bound_flags, _flag_buffers.split_flags, _flag_buffers.draw_flags,
                                       _active_matrix, _projection_buffer, reyes_config.bound_n_split_limit());
        _ready = _queue.enq_kernel(*_bound_kernel_gregory, round_up_by(_patch_count, 64), 64, "bound patches", ready | _ready);
        break;
    }


    
    prefix_sum_ready = _prefix_sum.apply(_patch_count, _queue,
                                        _flag_buffers.split_flags, _flag_buffers.split_flags, _split_ranges_cnt_buffer,
                                        _ready);
    prefix_sum_ready = _prefix_sum.apply(_patch_count, _queue,
                                        _flag_buffers.draw_flags, _flag_buffers.draw_flags, _out_range_cnt_buffer,
                                        prefix_sum_ready);

    mapping_ready = _queue.enq_map_buffer(_out_range_cnt_buffer, CL_MAP_READ, "buffer map", prefix_sum_ready);
    mapping_ready = _queue.enq_map_buffer(_split_ranges_cnt_buffer, CL_MAP_READ, "buffer map", mapping_ready);
    
    _move_kernel->set_args(_patch_count,
                           _read_buffers->pids, _read_buffers->depths, _read_buffers->mins, _read_buffers->maxs,
                           _flag_buffers.bound_flags, _flag_buffers.draw_flags, _flag_buffers.split_flags, 
                           _write_buffers->pids, _write_buffers->depths, _write_buffers->mins, _write_buffers->maxs,
                           _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer);
    _ready = _queue.enq_kernel(*_move_kernel, round_up_by(_patch_count, 64), 64, "split patches", _ready | prefix_sum_ready);
              
    _queue.wait_for_events(mapping_ready);
    mapping_ready = CL::Event();
    
    int draw_count = _out_range_cnt_buffer.host_ref<int>();
    int split_count = _split_ranges_cnt_buffer.host_ref<int>(); 
    _patch_count = split_count * 2;

    statistics.add_bounds(_patch_count);

    _queue.enq_unmap_buffer(_out_range_cnt_buffer, "buffer map", CL::Event());
    _queue.enq_unmap_buffer(_split_ranges_cnt_buffer, "buffer map", CL::Event());
    
    statistics.add_patches(draw_count);
    statistics.inc_pass_count(1);
    _user_event.end();
    
    // Swap ping-pong buffers
    std::swap(_read_buffers, _write_buffers);
    
    return {reyes_config.dummy_render() ? 0 : (size_t)draw_count,
            _active_patch_type, *_active_patch_buffer,
            _out_pids_buffer, _out_mins_buffer, _out_maxs_buffer,
            _ready};
}
