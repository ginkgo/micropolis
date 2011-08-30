#include "Renderer.h"

#include "Patch.h"
#include "OpenCL.h"
#include "Framebuffer.h"
#include "Projection.h"

#include "Statistics.h"
#include "Config.h"
namespace Reyes
{

    Renderer::Renderer(CL::Device& device,
                       CL::CommandQueue& queue,
                       Framebuffer& framebuffer,
                       Statistics& statistics) :
        _queue(queue),
        _framebuffer(framebuffer),
        _statistics(statistics),
        _control_points(16 * config.reyes_patches_per_pass()),
        _patch_count(0),
        _patch_buffer(device, _control_points.size() * sizeof(vec4), CL_MEM_READ_ONLY),
        _dice_kernel(device, "reyes.cl", "dice")
    {

    }

    Renderer::~Renderer()
    {

    }

    void Renderer::prepare()
    {
        _framebuffer.acquire(_queue);
        _framebuffer.clear(_queue);

        _statistics.start_render();
    }

    void Renderer::finish()
    {
        _statistics.end_render();
        _queue.finish();
        _framebuffer.release(_queue);
        _queue.finish();
    }

    void Renderer::set_projection(const Projection& projection)
    {
        mat4 proj;
        projection.calc_projection(proj);
        _dice_kernel.set_arg(4, proj);
        _dice_kernel.set_arg(5, projection.get_viewport());
    }

    void Renderer::draw_patch (const BezierPatch& patch) 
    {
        size_t cp_index = _patch_count * 16;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                _control_points[cp_index] = vec4(patch.P[i][j], 1);
                ++cp_index;
            }
        }

        _patch_count++;
        
        if (_patch_count >= config.reyes_patches_per_pass()) {
            flush();
        }
    
        _statistics.inc_patch_count();
    }

    void Renderer::flush()
    {
        if (_patch_count == 0) {
            return;
        }

        _queue.enq_write_buffer(_patch_buffer, 
                                _control_points.data(), 
                                sizeof(vec4) * _patch_count * 16);

        _dice_kernel.set_arg_r(0, _patch_buffer);
        _dice_kernel.set_arg_r(1, _framebuffer.get_buffer());
        _dice_kernel.set_arg  (2, _framebuffer.get_tile_size());
        _dice_kernel.set_arg  (3, _framebuffer.get_grid_size());

        _queue.enq_kernel(_dice_kernel,
                          ivec3(config.reyes_patch_size()+config.dice_group_width(), 
                                config.reyes_patch_size()+config.dice_group_width(), _patch_count),
                          ivec3(config.dice_group_width(), 
                                config.dice_group_width(), 1));

        // TODO: Make this more elegant
        _queue.finish();

        _patch_count = 0;
    }

}
