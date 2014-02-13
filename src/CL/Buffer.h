
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

        string _use;
        bool _shared;

        Buffer(Device& device, cl_mem_flags flags, const string& use="unkown") : _device(&device), _flags(flags), _size(0), _buffer(0), _use(use), _shared(false) {};
        
    public:
        
        
        Buffer(Device& device, size_t size, cl_mem_flags flags, const string& use="unknown");
        Buffer(Device& device, GLuint GL_buffer, const string& use="unknown");
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

        TransferBuffer(Device& device, size_t size, cl_mem_flags flags, const string& use="unknown");
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
