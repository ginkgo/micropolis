/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#include "Renderer.h"

#include "Patch.h"
#include "OpenCL.h"
#include "Framebuffer.h"
#include "Projection.h"

#include "Statistics.h"
#include "Config.h"
namespace Reyes
{

    Renderer::Renderer() :
        _device(config.platform_id(), config.device_id()),
        _queue(_device),
        _framebuffer(_device, config.window_size(), config.framebuffer_tile_size(), glfwGetCurrentContext()),
        _active_patch_buffer(0),
        _patch_buffers(config.patch_buffer_count()),
        _back_buffer(0),
        _patch_count(0),
        _max_block_count(square(config.reyes_patch_size()/8) * config.reyes_patches_per_pass()),
        _pos_grid(_device, 
                  config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(vec4),
                  CL_MEM_READ_WRITE),
        _pxlpos_grid(_device, 
                     config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(ivec2),
                     CL_MEM_READ_WRITE),
        _color_grid(_device, 
                    config.reyes_patches_per_pass() * square(config.reyes_patch_size()) * sizeof(vec4),
                    CL_MEM_READ_WRITE),
        _depth_grid(_device, 
                    config.reyes_patches_per_pass() * square(config.reyes_patch_size()+1) * sizeof(float),
                    CL_MEM_READ_WRITE),
        _block_index(_device, _max_block_count * sizeof(ivec4), CL_MEM_READ_WRITE),
        _tile_locks(_device,
                    _framebuffer.size().x/8 * _framebuffer.size().y/8 * sizeof(cl_int), CL_MEM_READ_WRITE),
		_depth_buffer(_device, _framebuffer.size().x * _framebuffer.size().y * sizeof(cl_int), CL_MEM_READ_WRITE),
        _reyes_program()
    {
        _reyes_program.set_constant("TILE_SIZE", _framebuffer.get_tile_size());
        _reyes_program.set_constant("GRID_SIZE", _framebuffer.get_grid_size());
        _reyes_program.set_constant("PATCH_SIZE", (int)config.reyes_patch_size());
        _reyes_program.set_constant("VIEWPORT_MIN_PIXEL", ivec2(0,0));
        _reyes_program.set_constant("VIEWPORT_MAX_PIXEL", _framebuffer.size());
        _reyes_program.set_constant("VIEWPORT_SIZE_PIXEL", _framebuffer.size());
        _reyes_program.set_constant("MAX_BLOCK_COUNT", _max_block_count);
        _reyes_program.set_constant("MAX_BLOCK_ASSIGNMENTS", config.max_block_assignments());
        _reyes_program.set_constant("FRAMEBUFFER_SIZE", _framebuffer.size());
        _reyes_program.set_constant("BACKFACE_CULLING", config.backface_culling());
        _reyes_program.set_constant("CLEAR_COLOR", config.clear_color());
        _reyes_program.set_constant("CLEAR_DEPTH", 1.0f);
        _reyes_program.set_constant("PXLCOORD_SHIFT", config.subpixel_bits());
                
        _reyes_program.compile(_device, "reyes.cl");

        _dice_kernel.reset(_reyes_program.get_kernel("dice"));
        _shade_kernel.reset(_reyes_program.get_kernel("shade"));
        _sample_kernel.reset(_reyes_program.get_kernel("sample"));
        _init_tile_locks_kernel.reset(_reyes_program.get_kernel("init_tile_locks"));
        _clear_depth_buffer_kernel.reset(_reyes_program.get_kernel("clear_depth_buffer"));

        _dice_kernel->set_arg_r(1, _pos_grid);
        _dice_kernel->set_arg_r(2, _pxlpos_grid);
        _dice_kernel->set_arg_r(4, _depth_grid);

        _shade_kernel->set_arg_r(0, _pos_grid);
        _shade_kernel->set_arg_r(1, _pxlpos_grid);
        _shade_kernel->set_arg_r(2, _block_index);
        _shade_kernel->set_arg_r(3, _color_grid);

        _sample_kernel->set_arg_r(0, _block_index);
        _sample_kernel->set_arg_r(1, _pxlpos_grid);
        _sample_kernel->set_arg_r(2, _color_grid);
        _sample_kernel->set_arg_r(3, _depth_grid);
        _sample_kernel->set_arg_r(4, _tile_locks);
        _sample_kernel->set_arg_r(5, _framebuffer.get_buffer());
        _sample_kernel->set_arg_r(6, _depth_buffer);

        _clear_depth_buffer_kernel->set_arg_r(0, _depth_buffer);

        _patch_buffers.resize(config.patch_buffer_count());

        for (int i = 0; i < config.patch_buffer_count(); ++i) {
            PatchBuffer& buffer = _patch_buffers.at(i);

            if (config.patch_buffer_mode() == Config::PINNED) {
                buffer.host = NULL;
                buffer.buffer = new CL::Buffer(_device, _queue, config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2, 
                                               CL_MEM_READ_ONLY, &(buffer.host));
            } else {
                buffer.host = malloc(config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2);
                buffer.buffer = new CL::Buffer(_device, config.reyes_patches_per_pass() * 16 * sizeof(vec4) * 2, 
                                               CL_MEM_READ_ONLY, buffer.host);
            }

            assert (buffer.host != NULL);
            buffer.write_complete = CL::Event();
        }

        _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);
	    
        _init_tile_locks_kernel->set_arg_r(0, _tile_locks);
        _queue.enq_kernel(*_init_tile_locks_kernel, _framebuffer.size().x/8 * _framebuffer.size().y/8, 64, 
                          "init tile locks", CL::Event());
    }

    Renderer::~Renderer()
    {

        for (int i = 0; i < config.patch_buffer_count(); ++i) {
            delete _patch_buffers.at(i).buffer;

            if (config.patch_buffer_mode() == Config::UNPINNED) {
                free(_patch_buffers.at(i).host);
            }
        }
    }

    void Renderer::prepare()
    {
        CL::Event e = _framebuffer.acquire(_queue, CL::Event());
        e = _framebuffer.clear(_queue, e);
        _framebuffer_cleared = _queue.enq_kernel(*_clear_depth_buffer_kernel, 
                                                 _framebuffer.size().x * _framebuffer.size().y, 64, 
                                                 "clear depthbuffer", e);

        statistics.start_render();
    }

    void Renderer::finish()
    {
        
        flush();

        _framebuffer.release(_queue, _last_sample);

        _framebuffer.show();
        
        for (int i = 0; i < config.patch_buffer_count(); ++i) {
            _patch_buffers.at(i).write_complete = CL::Event();
        }

        _last_sample = CL::Event();
        _framebuffer_cleared = CL::Event();

        _active_patch_buffer = 0;
        _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);

        statistics.end_render();
    }

    void Renderer::set_projection(const Projection& projection)
    {
        mat4 proj;
        projection.calc_projection(proj);

        _dice_kernel->set_arg(3, proj);
    }

    void Renderer::draw_patch (const BezierPatch& patch) 
    {
        size_t cp_index = _patch_count * 16;

        for     (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
		        assert(cp_index < config.reyes_patches_per_pass()  * 16);
                _back_buffer[cp_index] = vec4(patch.P[i][j], 1);
                ++cp_index;
            }
        }

        _patch_count++;
        
        if (_patch_count >= config.reyes_patches_per_pass()) {
            statistics.stop_bound_n_split();

            flush();
            _queue.wait_for_events(_patch_buffers.at(_active_patch_buffer).write_complete);

            statistics.start_bound_n_split();
        }
    
        statistics.inc_patch_count();
    }

    void Renderer::flush()
    {
        if (_patch_count == 0) {
            return;
        }
 
        PatchBuffer& active  = _patch_buffers.at(_active_patch_buffer);
        _active_patch_buffer = (_active_patch_buffer+1) % config.patch_buffer_count();
        _back_buffer = (vec4*)(_patch_buffers.at(_active_patch_buffer).host);

        CL::Event e, f;
        e = _queue.enq_write_buffer(*(active.buffer), active.host, _patch_count * sizeof(vec4) * 16,
                                    "write patches", CL::Event());

        int patch_size  = config.reyes_patch_size();
        int group_width = config.dice_group_width();

        _dice_kernel->set_arg_r(0, *(active.buffer));
        e = _queue.enq_kernel(*_dice_kernel,
                              ivec3(patch_size + group_width, patch_size + group_width, _patch_count),
                              ivec3(group_width, group_width, 1),
                              "dice", e | _last_sample);
        e = _queue.enq_kernel(*_shade_kernel, ivec3(patch_size, patch_size, _patch_count),  ivec3(8, 8, 1),
                              "shade", e);
        e = _queue.enq_kernel(*_sample_kernel, ivec3(8,8,_patch_count * square(patch_size/8)), ivec3(8,8,1),
                              "sample", _framebuffer_cleared | e);
                        
        active.write_complete = e;
        _last_sample = e;
        _patch_count = 0;

        //_queue.flush();
        
    }

}
