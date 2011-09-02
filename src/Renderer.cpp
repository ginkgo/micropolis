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
                       Framebuffer& framebuffer) :
        _queue(queue),
        _framebuffer(framebuffer),
        _control_points(16 * config.reyes_patches_per_pass()),
        _patch_count(0),
        _patch_buffer(device, _control_points.size() * sizeof(vec4), CL_MEM_READ_ONLY),
        _grid_buffer(device, 
                     config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(vec4),
                     CL_MEM_READ_WRITE),
        _reyes_program()
    {
        _reyes_program.set_constant("TILE_SIZE", _framebuffer.get_tile_size());
        _reyes_program.set_constant("GRID_SIZE", _framebuffer.get_grid_size());
        _reyes_program.set_constant("PATCH_SIZE", config.reyes_patch_size());
        
        _reyes_program.compile(device, "reyes.cl");

        _dice_kernel.reset(_reyes_program.get_kernel("dice"));
        _shade_kernel.reset(_reyes_program.get_kernel("shade"));

        _dice_kernel->set_arg_r(0, _patch_buffer);
        _dice_kernel->set_arg_r(1, _grid_buffer);

        _shade_kernel->set_arg_r(0, _grid_buffer);
        _shade_kernel->set_arg_r(1, _framebuffer.get_buffer());
    }

    Renderer::~Renderer()
    {

    }

    void Renderer::prepare()
    {
        _framebuffer.acquire(_queue);
        _framebuffer.clear(_queue);

        statistics.start_render();
    }

    void Renderer::finish()
    {
        flush();
        _queue.finish();

        statistics.end_render();

        _framebuffer.release(_queue);
        _queue.finish();
    }

    void Renderer::set_projection(const Projection& projection)
    {
        mat4 proj;
        projection.calc_projection(proj);

        _shade_kernel->set_arg  (2, proj);
        _shade_kernel->set_arg  (3, projection.get_viewport());
    }

    void Renderer::draw_patch (const BezierPatch& patch) 
    {
        size_t cp_index = _patch_count * 16;

        for     (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                _control_points[cp_index] = vec4(patch.P[i][j], 1);
                ++cp_index;
            }
        }

        _patch_count++;
        
        if (_patch_count >= config.reyes_patches_per_pass()) {
            flush();
        }
    
        statistics.inc_patch_count();
    }

    void Renderer::flush()
    {
        if (_patch_count == 0) {
            return;
        }

        _queue.enq_write_buffer(_patch_buffer, 
                                _control_points.data(), 
                                sizeof(vec4) * _patch_count * 16);

        int patch_size  = config.reyes_patch_size();
        int group_width = config.dice_group_width();

        _queue.enq_kernel(*_dice_kernel,
                          ivec3(patch_size + group_width, patch_size + group_width, _patch_count),
                          ivec3(group_width, group_width, 1));


        _queue.enq_kernel(*_shade_kernel, 
                          ivec3(patch_size, patch_size, _patch_count), 
                          ivec3(8, 8, 1));
                                

        // TODO: Make this more elegant
        _queue.finish();

        _patch_count = 0;
    }

}
