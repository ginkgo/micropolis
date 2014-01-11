
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
        
        Device* _device;
        cl_mem_flags _flags;
        size_t _size;
        
        cl_mem _buffer;

        Buffer(Device& device, cl_mem_flags flags) : _device(&device), _flags(flags), _size(0), _buffer(0) {};
        
    public:
        
        
        Buffer(Device& device, size_t size, cl_mem_flags flags);
        Buffer(Device& device, GLuint GL_buffer);
        virtual ~Buffer();

        Buffer(Buffer&& other);
        Buffer& operator=(Buffer&& other);
        
        cl_mem get() const { return _buffer; }
        
        size_t get_size() const;

        virtual void resize(size_t new_size);
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

        TransferBuffer(Device& device, size_t size, cl_mem_flags flags);
        virtual ~TransferBuffer();

        TransferBuffer(TransferBuffer&&);
        TransferBuffer& operator=(TransferBuffer&&);

        virtual void resize(size_t new_size);

        void set_host_ptr(void* ptr) { _host_ptr = ptr; }
        
        void* void_ptr();
        template<typename T> T* host_ptr() { return (T*)void_ptr(); };
        template<typename T> T& host_ref() { return *((T*)void_ptr()); };
    };

    
}
