#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"

#include "Patch.h"
#include "OpenCL.h"

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
        
        CL::Buffer _patch_buffer;
        CL::Buffer _grid_buffer;

        CL::Program _reyes_program;

        scoped_ptr<CL::Kernel> _dice_kernel;
        scoped_ptr<CL::Kernel> _shade_kernel;

        public:

        Renderer(CL::Device& device, 
                 CL::CommandQueue& queue, 
                 Framebuffer& framebuffer);

        ~Renderer();

        void prepare();
        void finish();

        void set_projection(const Projection& projection);

        virtual void draw_patch (const BezierPatch& patch);

        private:

        void flush();
        
    };

}

#endif
