
#include "Reyes.h"

#include "OpenCL.h"

namespace Reyes
{

    OGLSharedFramebuffer::OGLSharedFramebuffer(CL::Device& device,
                                               const ivec2& size, int tile_size) :
        _size(size),
        _tile_size(tile_size),
        _grid_size(ceil((float)size.x/tile_size), ceil((float)size.y/tile_size)),
        _act_size(_grid_size * tile_size), 
        _shader("tex_draw"),
        _tex_buffer(_act_size.x * _act_size.y * sizeof(vec4), GL_RGBA32F),
        _shared(device.share_gl()),
        _clear_kernel(device, "framebuffer.cl", "clear"),
        _local(0)
    {
        if (_shared) {
            _cl_buffer = new CL::Buffer(device, _tex_buffer.get_buffer());
        } else {
            _cl_buffer = new CL::Buffer(device, _tex_buffer.get_size(), CL_MEM_READ_WRITE);
            _local = malloc(_tex_buffer.get_size());
        }

        _clear_kernel.set_arg(0, _cl_buffer->get());
        _clear_kernel.set_arg(1, vec4(0,0,1,1));
    }

    OGLSharedFramebuffer::~OGLSharedFramebuffer()
    {
        if (_local) {
            free(_local);
        }

        delete _cl_buffer;
    }

    void OGLSharedFramebuffer::acquire(CL::CommandQueue& queue)
    {
        if (_shared) {
            queue.enq_GL_acquire(_cl_buffer->get());
        } 
    }

    void OGLSharedFramebuffer::release(CL::CommandQueue& queue)
    {
        if (_shared) {
            queue.enq_GL_release(_cl_buffer->get());
        } else {

            queue.enq_read_buffer(*_cl_buffer, _local, _tex_buffer.get_size());
            queue.finish();

            _tex_buffer.load(_local);
        }
    }

    void OGLSharedFramebuffer::show()
    {
        _tex_buffer.bind();
        
        _shader.bind();

        _shader.set_uniform("framebuffer", _tex_buffer);
        _shader.set_uniform("bsize", _tile_size);
        _shader.set_uniform("gridsize", _grid_size);

        glBegin(GL_QUADS);
        glVertex2f(-1,-1);
        glVertex2f( 1,-1);
        glVertex2f( 1, 1);
        glVertex2f(-1, 1);
        glEnd();

        _shader.unbind();
        
        _tex_buffer.unbind();
    }
    
    void OGLSharedFramebuffer::clear(CL::CommandQueue& queue)
    {
        queue.enq_kernel(_clear_kernel, _size, ivec2(_tile_size, _tile_size));
    }
}
