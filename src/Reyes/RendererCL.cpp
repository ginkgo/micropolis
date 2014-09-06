#include "common.h"

#include "RendererCL.h"

#include "BoundNSplitCLBalanced.h"
#include "BoundNSplitCLCPU.h"
#include "BoundNSplitCLLocal.h"
#include "BoundNSplitCLMultipass.h"
#include "BoundNSplitCLBreadthFirst.h"
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

    , _framebuffer(_device, config.window_size(), reyes_config.framebuffer_tile_size(), glfwGetCurrentContext())

    , _patch_index(new PatchIndex())
    
    , _reyes_program()
    , _frame_event(_device, "frame")
{
    
    switch(reyes_config.bound_n_split_method()) {
    case ReyesConfig::BALANCED:
        _bound_n_split.reset(new BoundNSplitCLBalanced(_device, _bound_n_split_queue, _patch_index));
        break;
    case ReyesConfig::CPU:
        _bound_n_split.reset(new BoundNSplitCLCPU(_device, _bound_n_split_queue, _patch_index));
        break;
    case ReyesConfig::LOCAL:
        _bound_n_split.reset(new BoundNSplitCLLocal(_device, _bound_n_split_queue, _patch_index));
        break;
    case ReyesConfig::BREADTHFIRST:
        _bound_n_split.reset(new BoundNSplitCLBreadthFirst(_device, _bound_n_split_queue, _patch_index));
        break;
    default:
        cerr << "Configured bound&split method not supported. Falling back to multipass" << endl;
    case ReyesConfig::MULTIPASS:
        _bound_n_split.reset(new BoundNSplitCLMultipass(_device, _bound_n_split_queue, _patch_index));
        break;
    }
    
    _reyes_program.set_constant("TILE_SIZE", _framebuffer.get_tile_size());
    _reyes_program.set_constant("GRID_SIZE", _framebuffer.get_grid_size());
    _reyes_program.set_constant("VIEWPORT_MIN_PIXEL", ivec2(0,0));
    _reyes_program.set_constant("VIEWPORT_MAX_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("VIEWPORT_SIZE_PIXEL", _framebuffer.size());
    _reyes_program.set_constant("FRAMEBUFFER_SIZE", _framebuffer.size());
    _reyes_program.set_constant("BACKFACE_CULLING", reyes_config.backface_culling());
    _reyes_program.set_constant("CLEAR_COLOR", reyes_config.clear_color());
    _reyes_program.set_constant("CLEAR_DEPTH", 1.0f);
                
    _reyes_program.compile(_device, "reyes.cl");
}


Reyes::RendererCL::~RendererCL()
{
}



void Reyes::RendererCL::prepare()
{
    _frame_event.begin(CL::Event());
    
    CL::Event e = _framebuffer.acquire(_framebuffer_queue, CL::Event());
    e = _framebuffer.clear(_framebuffer_queue, e);

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
    // We can't handle more patches on the fly atm
    int patch_count = std::min<int>(reyes_config.reyes_patches_per_pass(), batch.patch_count);
    
    if (patch_count == 0) {
        return _last_batch;
    }

    CL::Event e = ready;

    _rasterization_queue.flush();

    return e;
}
