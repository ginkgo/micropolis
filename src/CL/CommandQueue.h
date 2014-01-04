#pragma once

#include "common.h"

#include "Buffer.h"
#include "Device.h"
#include "Exception.h"

#include <CL/opencl.h>
#include <map>


namespace CL
{
    class Kernel;
    class Device;
    class Event;
    class Buffer;
    class TransferBuffer;

    class CommandQueue : public noncopyable
    {

        Device& _parent_device;

        cl_command_queue _queue;
        const string _name;

        vector<cl_event> _event_pad;
        cl_event* _event_pad_ptr;

    public:

        CommandQueue(Device& device, const string& name);
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

        void  map_buffer   (TransferBuffer& buffer, cl_map_flags map_flags, const string& name, const Event& events);
        Event enq_map_buffer   (TransferBuffer& buffer, cl_map_flags map_flags, const string& name, const Event& events, bool blocking=false);
        Event enq_unmap_buffer (TransferBuffer& buffer, const string& name, const Event& events);
        
        Event enq_write_buffer(Buffer& buffer, void* src, size_t length, size_t offset,
                               const string& name, const Event& events);
        Event enq_write_buffer(Buffer& buffer, void* src, size_t length,
                               const string& name, const Event& events);
        
        Event enq_read_buffer (Buffer& buffer, void* dst, size_t length, size_t offset,
                               const string& name, const Event& events);
        Event enq_read_buffer (Buffer& buffer, void* dst, size_t length,
                               const string& name, const Event& events);

        template<typename T>
        Event enq_fill_buffer(Buffer& buffer, const T& pattern, size_t length,
                              const string& name, const Event& events);
        
        void wait_for_events (const Event& events);

        void finish();
        void flush();
        
    };


    // --- Template implementation

    
    template<typename T>
    Event CommandQueue::enq_fill_buffer(Buffer& buffer, const T& pattern, size_t length,
                                        const string& name, const Event& events)
    {
        size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
        cl_event e;
        cl_int status;

        status = clEnqueueFillBuffer(_queue, buffer.get(),
                                     &pattern, sizeof(T),
                                     0, length * sizeof(T),
                                     cnt, _event_pad_ptr, &e);

        //cout << name << endl;
        OPENCL_ASSERT(status);

        return _parent_device.insert_event(name, _name, e, events);
    }
    
}
