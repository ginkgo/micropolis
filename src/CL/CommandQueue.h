#pragma once

#include "common.h"
#include <CL/opencl.h>

#include <map>

namespace CL
{
    class Kernel;
    class Device;
    class Event;
    class Buffer;

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
    
}
