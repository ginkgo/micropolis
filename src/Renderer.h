#ifndef RENDERER_H
#define RENDERER_H

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
        Statistics& _statistics;

        vector<vec4> _control_points;

        size_t _patch_count;
        
        CL::Buffer _patch_buffer;

        CL::Kernel _dice_kernel;

        public:

        Renderer(CL::Device& device, 
                 CL::CommandQueue& queue, 
                 Framebuffer& framebuffer,
                 Statistics& statistics);

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
