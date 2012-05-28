/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#ifndef OPENCL_H
#define OPENCL_H

#include "common.h"
#include "CL/opencl.h"
#include "Texture.h"

#include <map>
#include <sstream>

namespace CL
{

    class CommandQueue;

    class Device : public noncopyable
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

    class ImageBuffer : public noncopyable
    {
        cl_mem _buffer;

        public:

        ImageBuffer(Device& device, GL::Texture& texture, cl_mem_flags flags);
        ~ImageBuffer();

        cl_mem get() { return _buffer; }
    };

    class Buffer : public noncopyable
    {
        cl_mem _buffer;

        public:

        Buffer(Device& device, size_t size, cl_mem_flags flags);
        Buffer(Device& device, size_t size, cl_mem_flags flags, void* host_ptr);
        Buffer(Device& device, CommandQueue& queue, size_t size, cl_mem_flags flags, void** host_ptr);
        Buffer(Device& device, GLuint GL_buffer);
        ~Buffer();

        cl_mem get() { return _buffer; }
        size_t get_size() const;
    };

    class Kernel : public noncopyable
    {
        cl_kernel _kernel;
        cl_program _program;

        public:

        Kernel(Device& device, const string& filename, const string& kernelname);
        Kernel(cl_program program, cl_device_id device, const string& kernelname);
        ~Kernel();

        template<typename T> void set_arg  (cl_uint arg_index, T buffer);
        template<typename T> void set_arg_r(cl_uint arg_index, T& buffer);

        cl_kernel get() { return _kernel;}
    };

    class Program : public noncopyable
    {
	cl_device_id _device;
        cl_program _program;
        std::stringstream* _source_buffer;

        public:

        Program();
        ~Program();

        void set_constant(const string& name, int value);
        void set_constant(const string& name, size_t value);
        void set_constant(const string& name, float value);
        void set_constant(const string& name, ivec2 value);
        void set_constant(const string& name, vec4 value);

        void compile(Device& device, const string& filename);

        Kernel* get_kernel(const string& name);
    };

    class Event : public noncopyable
    {
        static const int MAX_ID_COUNT = 16;
        
        long _ids[MAX_ID_COUNT];
        size_t _count;

        public:

        Event();
        Event(long id);
        Event(const Event& event);

        Event operator  | (const Event& other) const;
        Event& operator = (const Event& other);
        const size_t get_id_count() const;
        const long* get_ids() const;        
    };

    class CommandQueue : public noncopyable
    {
        struct EventIndex
        {
            long id;
            string name;
            cl_event event;
        };


        cl_command_queue _queue;

        std::map<long, EventIndex> _events;
        std::vector<cl_event> _event_pad;
        cl_event* _event_pad_ptr;
        long _id_count;

        Event insert_event(const string& name, cl_event event);
        size_t init_event_pad(const Event& event);

        public:

        CommandQueue(Device& device);
        ~CommandQueue();
        
        Event enq_kernel(Kernel& kernel, int global_size, int local_size,
                         const string& name, const Event& events);
        Event enq_kernel(Kernel& kernel, ivec2 global_size, ivec2 local_size,
                         const string& name, const Event& events);
        Event enq_kernel(Kernel& kernel, ivec3 global_size, ivec3 local_size,
                         const string& name, const Event& events);

        Event enq_GL_acquire(cl_mem buffer,
                             const string& name, const Event& events);
        Event enq_GL_release(cl_mem buffer,
                             const string& name, const Event& events);

        void* map_buffer  (Buffer& buffer);
        void  unmap_buffer(Buffer& buffer, void* mapped);

        Event enq_write_buffer(Buffer& buffer, void* src, size_t length, size_t offset,
                               const string& name, const Event& events);
        Event enq_read_buffer (Buffer& buffer, void* dst, size_t length, size_t offset,
                               const string& name, const Event& events);

        Event enq_write_buffer(Buffer& buffer, void* src, size_t length,
                               const string& name, const Event& events);
        Event enq_read_buffer (Buffer& buffer, void* dst, size_t length,
                               const string& name, const Event& events);

        void wait_for_events (const Event& events);

        void finish();
        void flush();
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

    template<>
    inline void Kernel::set_arg_r<Buffer>(cl_uint arg_index, Buffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }
}
#endif
