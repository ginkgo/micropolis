#include "common.h"

#include "RendererCL.h"

#include "BoundNSplitCLCPU.h"
#include "CL/OpenCL.h"
#include "Config.h"
#include "CLConfig.h"
#include "ReyesConfig.h"
#include "Framebuffer.h"
#include "PatchIndex.h"
#include "Projection.h"
#include "Statistics.h"

#define _framebuffer_queue _rasterization_queue
#define _bound_n_split_queue _rasterization_queue

Reyes::RendererCL::RendererCL()
    : _device(cl_config.opencl_device_id().x, cl_config.opencl_device_id().y)
      
    // , _framebuffer_queue(_device, "framebuffer")
    // , _bound_n_split_queue(_device, "bound & split")
    , _rasterization_queue(_device, "rasterization")

    , _framebuffer(_device, reyes_config.window_size(), reyes_config.framebuffer_tile_size(), glfwGetCurrentContext())

    , _patch_index(new PatchIndex())
      
    , _max_block_count(square(reyes_config.reyes_patch_size()/8) * reyes_config.reyes_patches_per_pass())
    , _pos_grid(_device, 
                reyes_config.reyes_patches_per_pass() * square(reyes_config.reyes_patch_size()+1) * sizeof(vec4),
                CL_MEM_READ_WRITE, "grid-data")
    , _pxlpos_grid(_device, 
                   reyes_config.reyes_patches_per_pass() * square(reyes_config.reyes_patch_size()+1) * sizeof(ivec2),
                   CL_MEM_READ_WRITE, "grid-data")
    , _color_grid(_device, 
                  reyes_config.reyes_patches_per_pass() * square(reyes_config.reyes_patch_size()) * sizeof(vec4),
                  CL_MEM_READ_WRITE, "grid-data")
    , _depth_grid(_device, 
                  reyes_config.reyes_patches_per_pass() * square(reyes_config.reyes_patch_size()+1) * sizeof(float),
                  CL_MEM_READ_WRITE, "grid-data")
    , _block_index(_device, _max_block_count * sizeof(ivec4), CL_MEM_READ_WRITE, "block-index")
    , _tile_locks(_device,
                  _framebuffer.size().x/8 * _framebuffer.size().y/8 * sizeof(cl_int), CL_MEM_READ_WRITE, "tile-locks")
	, _depth_buffer(_device, _framebuffer.size().x * _framebuffer.size().y * sizeof(cl_int), CL_MEM_READ_WRITE, "framebuffer")
    , _reyes_program()
    , _frame_event(_device, "frame")
{
    
    switch(reyes_config.bound_n_split_method()) {
    default:
        cerr << "Configured bound&split method not supported. Falling back to CPU" << endl;
    // case ReyesConfig::BALANCED:
    //     _bound_n_split.reset(new BoundNSplitCLBalanced(_device, _bound_n_split_queue, _patch_index));
    //     break;
    case ReyesConfig::CPU:
        _bound_n_split.reset(new BoundNSplitCLCPU(_device, _bound_n_split_queue, _patch_index));
        break;
    // case ReyesConfig::LOCAL:
    //     _bound_n_split.reset(new BoundNSplitCLLocal(_device, _bound_n_split_queue, _patch_index));
    //     break;
    // case ReyesConfig::BREADTHFIRST:
    //     _bound_n_split.reset(new BoundNSplitCLBreadthFirst(_device, _bound_n_split_queue, _patch_index));
    //     break;
    // case ReyesConfig::MULTIPASS:
    //     _bound_n_split.reset(new BoundNSplitCLMultipass(_device, _bound_n_split_queue, _patch_index));
    //     break;
    }

    for (CL::Program* program : {&_reyes_program, &_dice_bezier_program, &_dice_gregory_program}) {
        program->set_constant("TILE_SIZE", _framebuffer.get_tile_size());
        program->set_constant("GRID_SIZE", _framebuffer.get_grid_size());
        program->set_constant("PATCH_SIZE", (int)reyes_config.reyes_patch_size());
        program->set_constant("VIEWPORT_MIN_PIXEL", ivec2(0,0));
        program->set_constant("VIEWPORT_MAX_PIXEL", _framebuffer.size());
        program->set_constant("VIEWPORT_SIZE_PIXEL", _framebuffer.size());
        program->set_constant("MAX_BLOCK_COUNT", _max_block_count);
        program->set_constant("FRAMEBUFFER_SIZE", _framebuffer.size());
        program->set_constant("BACKFACE_CULLING", reyes_config.backface_culling());
        program->set_constant("CLEAR_COLOR", reyes_config.clear_color());
        program->set_constant("CLEAR_DEPTH", 1.0f);
        program->set_constant("PXLCOORD_SHIFT", reyes_config.subpixel_bits());
        program->set_constant("DISPLACEMENT", reyes_config.displacement());
    }
                
    _reyes_program.compile(_device, "reyes_old.cl");

    _shade_kernel.reset(_reyes_program.get_kernel("shade"));

    _sample_kernel.reset(_reyes_program.get_kernel("sample"));
    
    _dice_bezier_program.define("eval_patch", "eval_bezier_patch");
    _dice_bezier_program.compile(_device, "dice.cl");
    _dice_bezier_kernel.reset(_dice_bezier_program.get_kernel("dice"));

    _dice_gregory_program.define("eval_patch", "eval_gregory_patch");
    _dice_gregory_program.compile(_device, "dice.cl");
    _dice_gregory_kernel.reset(_dice_gregory_program.get_kernel("dice"));

    _rasterization_queue.enq_fill_buffer<cl_int>(_tile_locks,
                                                 1, _framebuffer.size().x/8 * _framebuffer.size().y/8,
                                                 "tile lock init", CL::Event());
}


Reyes::RendererCL::~RendererCL()
{
}



void Reyes::RendererCL::prepare()
{
    _frame_event.begin(CL::Event());
    
    CL::Event e = _framebuffer.acquire(_framebuffer_queue, CL::Event());
    e = _framebuffer.clear(_framebuffer_queue, e);

    _framebuffer_cleared =
        _framebuffer_queue.enq_fill_buffer<cl_int>(_depth_buffer,
                                                   0x7fffffff, _framebuffer.size().x * _framebuffer.size().y,
                                                   "clear depthbuffer", e);

    _framebuffer_queue.flush();
    statistics.start_render();

}


void Reyes::RendererCL::finish()
{
    _bound_n_split->finish();
    
    _framebuffer.release(_framebuffer_queue, _last_batch);
    _framebuffer.show();

    _frame_event.end();
    _device.release_events();
    
    _last_batch = CL::Event();
    _framebuffer_cleared = CL::Event();
        
    statistics.end_render();
}



bool Reyes::RendererCL::are_patches_loaded(void* patches_handle)
{
    return _patch_index->are_patches_loaded(patches_handle);
}


void Reyes::RendererCL::load_patches(void* patches_handle, const vector<vec3>& patch_data, PatchType patch_type)
{
    _patch_index->load_patches(patches_handle, patch_data, patch_type);
}


void Reyes::RendererCL::draw_patches(void* patches_handle,
                                     const mat4& matrix,
                                     const Projection* projection,
                                     const vec4& color)
{
    mat4 proj;
    projection->calc_projection(proj);

    _bound_n_split->init(patches_handle, matrix, projection);

    PatchType patch_type = _patch_index->get_patch_type(patches_handle);
    
    while (!_bound_n_split->done()) {
        
        Batch batch = _bound_n_split->do_bound_n_split(_last_batch);

        _last_batch = send_batch(batch, matrix, proj, color, patch_type, batch.transfer_done | _last_batch);
    }
}


CL::Event Reyes::RendererCL::send_batch(Reyes::Batch& batch,
                                        const mat4& matrix, const mat4& proj, const vec4& color, PatchType patch_type,
                                        const CL::Event& ready)
{
    
    // We can't handle more patches on the fly atm
    int patch_count = std::min<int>(reyes_config.reyes_patches_per_pass(), batch.patch_count);
    
    if (patch_count == 0) {
        return _last_batch;
    }

    CL::Event e;
    
    const int patch_size  = reyes_config.reyes_patch_size();
    const int group_width = reyes_config.dice_group_width();

    // DICE
    switch (patch_type) {
    case BEZIER:
        _dice_bezier_kernel->set_args(batch.patch_buffer, batch.patch_ids, batch.patch_min, batch.patch_max,
                                      _pos_grid, _pxlpos_grid, _depth_grid,
                                      matrix, proj);
        
        e = _rasterization_queue.enq_kernel(*_dice_bezier_kernel,
                                            ivec3(patch_size + group_width, patch_size + group_width, patch_count),
                                            ivec3(group_width, group_width, 1),
                                            "dice", ready);
        break;
    case GREGORY:
        _dice_gregory_kernel->set_args(batch.patch_buffer, batch.patch_ids, batch.patch_min, batch.patch_max,
                                      _pos_grid, _pxlpos_grid, _depth_grid,
                                      matrix, proj);
        
        e = _rasterization_queue.enq_kernel(*_dice_gregory_kernel,
                                            ivec3(patch_size + group_width, patch_size + group_width, patch_count),
                                            ivec3(group_width, group_width, 1),
                                            "dice", ready);
        break;
    }
    
    
    // SHADE
    _shade_kernel->set_args(_pos_grid, _pxlpos_grid, _block_index, _color_grid, color);
    e = _rasterization_queue.enq_kernel(*_shade_kernel, ivec3(patch_size, patch_size, patch_count),  ivec3(8,8,1),
                                        "shade", e);

    // SAMPLE
    _sample_kernel->set_args(_block_index, _pxlpos_grid, _color_grid, _depth_grid,
                             _tile_locks, _framebuffer.get_buffer(), _depth_buffer);
    e = _rasterization_queue.enq_kernel(*_sample_kernel, ivec3(8,8,patch_count * square(patch_size/8)), ivec3(8,8,1),
                                        "sample", _framebuffer_cleared | e);
    _framebuffer_cleared = CL::Event();

    _rasterization_queue.flush();

    return e;
}
