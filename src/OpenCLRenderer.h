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

#include "CL/OpenCL.h"
#include "Framebuffer.h"
#include "PatchIndex.h"
#include "Renderer.h"

namespace Reyes
{
    class Batch;
    class PatchIndex;
    class OpenCLBoundNSplit;

    class OpenCLRenderer : public Renderer
    {
        
        CL::Device _device;
        
        // CL::CommandQueue _framebuffer_queue;
        // CL::CommandQueue _bound_n_split_queue;
        CL::CommandQueue _rasterization_queue;
        
        OGLSharedFramebuffer _framebuffer;

        shared_ptr<PatchIndex> _patch_index;
        shared_ptr<OpenCLBoundNSplit> _bound_n_split;
        
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

        CL::Event _last_batch;
        CL::Event _framebuffer_cleared;
        CL::UserEvent _frame_event;


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

        virtual void dump_trace() { _device.dump_trace(); }

    private:

        void set_projection(const Projection& projection);
        void draw_patch(const BezierPatch& patch);
        CL::Event send_batch(Reyes::Batch& batch, const mat4& matrix, const mat4& proj, const vec4& color, const CL::Event& ready);

    };
}
