#ifndef RENDERER_H
#define RENDERER_H

#include "Patch.h"

namespace CL
{
    class Device;
    class CommandQueue;
}

namespace Reyes
{

    class Framebuffer;
    class Statistics;
    
    class Renderer : public PatchDrawer
    {

        CL::CommandQueue& _queue;
        Framebuffer& _framebuffer;
        Statistics& _statistics;

        public:

        Renderer(CL::Device& device, 
                 CL::CommandQueue& queue, 
                 Framebuffer& framebuffer,
                 Statistics& statistics);

        ~Renderer();

        void prepare();
        void finish();

        virtual void draw_patch (const BezierPatch& patch);
        
    };

}

#endif
