#include "common.h"

#include "RendererCL.h"

#include "CL/OpenCL.h"
#include "Config.h"
#include "Framebuffer.h"
#include "BoundNSplitCL.h"
#include "PatchIndex.h"
#include "Projection.h"
#include "Statistics.h"

#define _framebuffer_queue _rasterization_queue
#define _bound_n_split_queue _rasterization_queue

Reyes::RendererCL::RendererCL()
    : _device(config.opencl_device_id().x, config.opencl_device_id().y)
      
    // , _framebuffer_queue(_device, "framebuffer")
    // , _bound_n_split_queue(_device, "bound & split")
    , _rasterization_queue(_device, "rasterization")

    , _framebuffer(_device, config.window_size(), config.framebuffer_tile_size(), glfwGetCurrentContext())

    , _patch_index(new PatchIndex())
      
    , _max_block_count(square(config.reyes_patch_size()/8) * config.reyes_patches_per_pass())
    , _pos_grid(_device, 
                config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(vec4),
                CL_MEM_READ_WRITE)
    , _pxlpos_grid(_device, 
                   config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(ivec2),
                   CL_MEM_READ_WRITE)
    , _color_grid(_device, 
                  config.reyes_patches_per_pass() * square(config.reyes_patch_size()) * sizeof(vec4),
                  CL_MEM_READ_WRITE)
    , _depth_grid(_device, 
                  config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(float),
                  CL_MEM_READ_WRITE)
    , _block_index(_device, _max_block_count * sizeof(ivec4), CL_MEM_READ_WRITE)
    , _tile_locks(_device,
                  _framebuffer.size().x/8 * _framebuffer.size().y/8 * sizeof(cl_int), CL_MEM_READ_WRITE)
	, _depth_buffer(_device, _framebuffer.size().x * _framebuffer.size().y * sizeof(cl_int), CL_MEM_READ_WRITE)
    , _reyes_program()
    , _frame_event(_device, "frame")
{
    
    switch(config.bound_n_split_method()) {
    case Config::CPU:
        _bound_n_split.reset(new BoundNSplitCLCPU(_device, _bound_n_split_queue, _patch_index));
        break;
    case Config::MULTIPASS:
        _bound_n_split.reset(new BoundNSplitCLMultipass(_device, _bound_n_split_queue, _patch_index));
        break;
    case Config::LOCAL:
        _bound_n_split.reset(new BoundNSplitCLLocal(_device, _bound_n_split_queue, _patch_index));
        break;
    }
    
    _reyes_program.set_constant("TILE_SIZE", _framebuffer.get_tile_size());
    _reyes_program.set_constant("GRID_SIZE", _framebuffer.get_grid_size());
    _reyes_program.set_constant("PATCH_SIZE", (int)config.reyes_patch_size());
    _reyes_program.set_constant("VIEWPORT_MIN_PIXEL", ivec2(0,0));
    _reyes_program.set_constant("VIEWPORT_MAX_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("VIEWPORT_SIZE_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("MAX_BLOCK_COUNT", _max_block_count);
    _reyes_program.set_constant("FRAMEBUFFER_SIZE", _framebuffer.size());
    _reyes_program.set_constant("BACKFACE_CULLING", config.backface_culling());
    _reyes_program.set_constant("CLEAR_COLOR", config.clear_color());
    _reyes_program.set_constant("CLEAR_DEPTH", 1.0f);
    _reyes_program.set_constant("PXLCOORD_SHIFT", config.subpixel_bits());
                
    _reyes_program.compile(_device, "reyes.cl");

    _dice_kernel.reset(_reyes_program.get_kernel("dice"));
    _shade_kernel.reset(_reyes_program.get_kernel("shade"));
    _sample_kernel.reset(_reyes_program.get_kernel("sample"));


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


void Reyes::RendererCL::load_patches(void* patches_handle, const vector<BezierPatch>& patch_data)
{
    _patch_index->load_patches(patches_handle, patch_data);
}


void Reyes::RendererCL::draw_patches(void* patches_handle,
                                         const mat4& matrix,
                                         const Projection* projection,
                                         const vec4& color)
{
    mat4 proj;
    projection->calc_projection(proj);

    _bound_n_split->init(patches_handle, matrix, projection);
    
    while (!_bound_n_split->done()) {
        
        Batch batch = _bound_n_split->do_bound_n_split(_last_batch);

        _last_batch = send_batch(batch, matrix, proj, color, batch.transfer_done | _last_batch);
    }
}


CL::Event Reyes::RendererCL::send_batch(Reyes::Batch& batch,
                                            const mat4& matrix, const mat4& proj, const vec4& color,
                                            const CL::Event& ready)
{
    int patch_count = batch.patch_count;
    
    if (patch_count == 0) {
        return _last_batch;
    }

    CL::Event e;
    
    const int patch_size  = config.reyes_patch_size();
    const int group_width = config.dice_group_width();

    // DICE
    _dice_kernel->set_args(batch.patch_buffer, batch.patch_ids, batch.patch_min, batch.patch_max,
                           _pos_grid, _pxlpos_grid, _depth_grid,
                           matrix, proj);    
    e = _rasterization_queue.enq_kernel(*_dice_kernel,
                                        ivec3(patch_size + group_width, patch_size + group_width, patch_count),
                                        ivec3(group_width, group_width, 1),
                                        "dice", ready);

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
