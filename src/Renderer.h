#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"

#include "Patch.h"
#include "OpenCL.h"

#include "PatchDrawer.h"

namespace Reyes
{

    class Framebuffer;
    class Statistics;
    
    class Renderer : public PatchDrawer
    {

        CL::CommandQueue& _queue;
        Framebuffer& _framebuffer;

        vector<vec4> _control_points;

        size_t _patch_count;
        size_t _max_block_count;

        CL::Buffer _patch_buffer;
        CL::Buffer _pos_grid;
        CL::Buffer _pxlpos_grid;
        CL::Buffer _color_grid;
        CL::Buffer _block_index;
        CL::Buffer _head_buffer;
        CL::Buffer _node_heap;
        
        CL::Program _reyes_program;

        scoped_ptr<CL::Kernel> _dice_kernel;
        scoped_ptr<CL::Kernel> _shade_kernel;
        scoped_ptr<CL::Kernel> _clear_heads_kernel;
        scoped_ptr<CL::Kernel> _assign_kernel;
        scoped_ptr<CL::Kernel> _sample_kernel;

        CL::Event _last_patch_write;
        CL::Event _last_dice;
        CL::Event _last_sample;
        CL::Event _framebuffer_acquire;

        public:

        Renderer(CL::Device& device, 
                 CL::CommandQueue& queue, 
                 Framebuffer& framebuffer);

        ~Renderer();

        virtual void prepare();
        virtual void finish();

        virtual void set_projection(const Projection& projection);
        virtual void draw_patch (const BezierPatch& patch);

        private:

        void flush();
        
    };

}

#endif
