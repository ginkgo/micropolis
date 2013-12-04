#pragma once

#include "common.h"

#include <CL/opencl.h>

#include "GL/Texture.h"

namespace CL
{
    class CommandQueue;
    class Device;

    class Buffer : public noncopyable
    {
    protected:
        
        cl_mem _buffer;

    public:
        
        Buffer(Device& device, size_t size, cl_mem_flags flags);
        Buffer(Device& device, size_t size, cl_mem_flags flags, void* host_ptr);
        Buffer(Device& device, CommandQueue& queue, size_t size, cl_mem_flags flags, void** host_ptr);
        Buffer(Device& device, GLuint GL_buffer);
        virtual ~Buffer();

        cl_mem get() const { return _buffer; }
        size_t get_size() const;
    };

    
    class ImageBuffer : public noncopyable
    {
        cl_mem _buffer;

        public:

        ImageBuffer(Device& device, GL::Texture& texture, cl_mem_flags flags);
        ~ImageBuffer();

        cl_mem get() const { return _buffer; }
    };

    
    class TransferBuffer : public Buffer
    {
        void* _host_ptr;

        public:

        TransferBuffer(Device& device, CommandQueue& queue, size_t size, cl_mem_flags flags);
        virtual ~TransferBuffer();

        void* host_ptr();
    };

    
}
