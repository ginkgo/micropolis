#ifndef OPENCL_H
#define OPENCL_H

#include "common.h"
#include "CL/opencl.h"
#include "Texture.h"

namespace CL
{

    class Device : public boost::noncopyable
    {
        cl_context   _context;
        cl_device_id _device;

        bool _share_gl;

        public:
        
        Device(int platform_index, int device_index);
        ~Device();

        cl_device_id get_device()  { return _device; }
        cl_context   get_context() { return _context; }

        bool share_gl() const { return _share_gl; }

        void print_info();
    };

    class ImageBuffer : public boost::noncopyable
    {
        cl_mem _buffer;

        public:

        ImageBuffer(Device& device, GL::Texture& texture, cl_mem_flags flags);
        ~ImageBuffer();

        cl_mem get() { return _buffer; }
    };

    class Buffer : public boost::noncopyable
    {
        cl_mem _buffer;

        public:

        Buffer(Device& device, size_t size, cl_mem_flags flags);
        Buffer(Device& device, GLuint GL_buffer);
        ~Buffer();

        cl_mem get() { return _buffer; }
        size_t get_size() const;
    };

    class Kernel : public boost::noncopyable
    {
        cl_kernel _kernel;
        cl_program _program;

        public:

        Kernel(Device& device, const string& filename, const string& kernelname);
        ~Kernel();

        template<typename T> void set_arg  (cl_uint arg_index, T buffer);
        template<typename T> void set_arg_r(cl_uint arg_index, T& buffer);

        cl_kernel get() { return _kernel;}
    };

    class CommandQueue : public boost::noncopyable
    {
        cl_command_queue _queue;

        public:

        CommandQueue(Device& device);
        ~CommandQueue();
        
        void enq_kernel(Kernel& kernel, ivec2 global_size, ivec2 local_size);
        void enq_kernel(Kernel& kernel, ivec3 global_size, ivec3 local_size);
        void enq_GL_acquire(ImageBuffer& buffer);
        void enq_GL_release(ImageBuffer& buffer);
        void enq_GL_acquire(Buffer& buffer);
        void enq_GL_release(Buffer& buffer);
        void enq_GL_acquire(cl_mem buffer);
        void enq_GL_release(cl_mem buffer);

        void* map_buffer  (Buffer& buffer);
        void  unmap_buffer(Buffer& buffer, void* mapped);

        void enq_write_buffer(Buffer& buffer, void* src, 
                              size_t length, size_t offset=0);
        void enq_read_buffer (Buffer& buffer, void* dst, 
                              size_t length, size_t offset=0);

        void finish();
    };

    class Exception
    {
        string _msg;
        string _file;
        int _line_no;

        public:

        Exception(cl_int err_code,   const string& file, int line_no);
        Exception(const string& msg, const string& file, int line_no);
        
        const string& msg();
        const string& file() { return _file; }
        const int line_no() { return _line_no; }
    };
    
}

#define OPENCL_EXCEPTION(error) throw CL::Exception((error), __FILE__, __LINE__)
#define OPENCL_ASSERT(error) if ((error)!= CL_SUCCESS) throw CL::Exception((error), __FILE__, __LINE__)

namespace CL
{
    template<typename T> 
    inline void Kernel::set_arg(cl_uint arg_index, T value) 
    {
        cl_int status;

        status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);
        OPENCL_ASSERT(status);
    }

    template<typename T> 
    inline void Kernel::set_arg_r(cl_uint arg_index, T& value) 
    {
        cl_int status;

        status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);
        OPENCL_ASSERT(status);
    }

    template<>
    inline void Kernel::set_arg_r<ImageBuffer>(cl_uint arg_index, ImageBuffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }
}
#endif
