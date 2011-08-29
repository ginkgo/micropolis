#include "Renderer.h"

#include "Patch.h"
#include "OpenCL.h"
#include "Framebuffer.h"

#include "Statistics.h"

namespace Reyes
{

    Renderer::Renderer(CL::Device& device,
                       CL::CommandQueue& queue,
                       Framebuffer& framebuffer,
                       Statistics& statistics) :
        _queue(queue),
        _framebuffer(framebuffer),
        _statistics(statistics)
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

    void Renderer::draw_patch (const BezierPatch& patch) 
    {
        _statistics.inc_patch_count();
    }

}
