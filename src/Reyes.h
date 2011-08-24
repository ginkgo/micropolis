#ifndef REYES_H
#define REYES_H

#include "common.h"

#include "Texture.h"
#include "Shader.h"
#include "OpenCL.h"

namespace Reyes
{
    class OGLSharedFramebuffer
    {
        ivec2 _size;
        int   _tile_size;
        ivec2 _grid_size;
        ivec2 _act_size;

        GL::Shader _shader;

        GL::TextureBuffer _tex_buffer;
        CL::Buffer* _cl_buffer;

        bool _shared;

        CL::Kernel _clear_kernel;

        void* _local;

        public:

        OGLSharedFramebuffer(CL::Device& device, 
                             const ivec2& size, int tile_size);
        virtual ~OGLSharedFramebuffer();
                             

        void acquire(CL::CommandQueue& queue);
        void release(CL::CommandQueue& queue);

        void clear(CL::CommandQueue& queue);

        void show();
    };
}

#endif
