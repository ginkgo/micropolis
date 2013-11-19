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

#pragma once

#include "Framebuffer.h"
#include "OpenCL.h"
#include "PatchDrawer.h"
#include "PatchesIndex.h"

namespace Reyes
{

    class OpenCLRenderer : public PatchDrawer
    {

        struct PatchBuffer
        {
            CL::Buffer* buffer;
            void* host;
            CL::Event write_complete;
        };
        
        CL::Device _device;
        CL::CommandQueue _queue;
        OGLSharedFramebuffer _framebuffer;

        size_t _active_patch_buffer;
        vector<PatchBuffer> _patch_buffers;
        vec4* _back_buffer;

        size_t _patch_count;
        size_t _max_block_count;

        CL::Buffer _pos_grid;
        CL::Buffer _pxlpos_grid;
        CL::Buffer _color_grid;
        CL::Buffer _depth_grid;
        CL::Buffer _block_index;
        CL::Buffer _tile_locks;
        CL::Buffer _depth_buffer;
        
        CL::Program _reyes_program;

        scoped_ptr<CL::Kernel> _dice_kernel;
        scoped_ptr<CL::Kernel> _shade_kernel;
        scoped_ptr<CL::Kernel> _sample_kernel;
        scoped_ptr<CL::Kernel> _init_tile_locks_kernel;
        scoped_ptr<CL::Kernel> _clear_depth_buffer_kernel;

        CL::Event _last_sample;
        CL::Event _framebuffer_cleared;


    public:

        OpenCLRenderer();
        ~OpenCLRenderer();

        virtual void prepare();
        virtual void finish();
        
        virtual bool are_patches_loaded(void* patches_handle);
        virtual void load_patches(void* patches_handle, const vector<BezierPatch>& patch_data);
        
        virtual void draw_patches(void* patches_handle,
                                  const mat4& matrix,
                                  const Projection* projection,
                                  const vec4& color);

    };
}
