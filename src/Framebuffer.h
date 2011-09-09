#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "common.h"

#include "Texture.h"
#include "Shader.h"
#include "OpenCL.h"

namespace Reyes
{
    
    class Framebuffer
    {
        
        protected:

        ivec2 _size;
        int   _tile_size;
        ivec2 _grid_size;
        ivec2 _act_size;

        GL::Shader _shader;
        CL::Kernel _clear_kernel;


        CL::Buffer* _cl_buffer;
       

        Framebuffer(CL::Device& device, const ivec2& size, int tile_size);


        public:

        virtual ~Framebuffer();


        void clear(CL::CommandQueue& queue);


        const CL::Buffer& get_buffer() { return *_cl_buffer; }
        ivec2 size() {return _size; }
        int   get_tile_size() { return _tile_size; }
        ivec2 get_grid_size() { return _grid_size; }

        virtual CL::Event acquire(CL::CommandQueue& queue, const CL::Event& e) = 0;
        virtual CL::Event void release(CL::CommandQueue& queue, const CL::Event& e) = 0;
        virtual void show() = 0;

    };


    class OGLSharedFramebuffer : public Framebuffer
    {

        GL::TextureBuffer _tex_buffer;
        bool _shared;
        void* _local;

        public:

        OGLSharedFramebuffer(CL::Device& device, const ivec2& size, int tile_size);

        virtual ~OGLSharedFramebuffer();


        virtual CL::Event acquire(CL::CommandQueue& queue, const CL::Event& e);
        virtual CL::Event release(CL::CommandQueue& queue, const CL::Event& e);
                             

        void show();
    };
}

#endif
