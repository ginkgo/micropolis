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

	struct PatchBuffer
	{
	    CL::Buffer* buffer;
	    void* host;
	    CL::Event write_complete;
	};

        CL::CommandQueue _queue;
        Framebuffer& _framebuffer;

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
        CL::Buffer _head_buffer;
        CL::Buffer _node_heap;
        
        CL::Program _reyes_program;

        scoped_ptr<CL::Kernel> _dice_kernel;
        scoped_ptr<CL::Kernel> _shade_kernel;
        scoped_ptr<CL::Kernel> _clear_heads_kernel;
        scoped_ptr<CL::Kernel> _assign_kernel;
        scoped_ptr<CL::Kernel> _sample_kernel;

        CL::Event _last_dice;
        CL::Event _last_sample;
        CL::Event _framebuffer_cleared;

        public:

        Renderer(CL::Device& device, 
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
