#include "CommandQueue.h"

#include "Config.h"

#include "Device.h"
#include "Event.h"
#include "Exception.h"
#include "Kernel.h"


CL::CommandQueue::CommandQueue(Device& device, const string& name)
    : _parent_device(device)
{
    cl_int status;
    _queue = clCreateCommandQueue(device.get_context(), device.get_device(),
                                  CL_QUEUE_PROFILING_ENABLE  | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, 
                                  &status);

    OPENCL_ASSERT(status);
}

CL::CommandQueue::~CommandQueue()
{
    clFinish(_queue);
    clReleaseCommandQueue(_queue);
}


CL::Event CL::CommandQueue::enq_kernel(Kernel& kernel, int global_size, int local_size,
                                       const string& name, const CL::Event& events)
{
    size_t offset[] = {0};
    size_t global[] = {(size_t)global_size};
    size_t local[]  = {(size_t)local_size};
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;

    cl_int status;
    status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                    1, offset, global, local,
                                    cnt, _event_pad_ptr, &e);
    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}


CL::Event CL::CommandQueue::enq_kernel(Kernel& kernel, ivec2 global_size, ivec2 local_size,
                                       const string& name, const CL::Event& events)
{
    size_t offset[] = {0,0};
    size_t global[] = {(size_t)global_size.x, (size_t)global_size.y};
    size_t local[]  = {(size_t)local_size.x, (size_t)local_size.y};
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;        

    cl_int status;
    status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                    2, offset, global, local,
                                    cnt, _event_pad_ptr, &e);

    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}

CL::Event CL::CommandQueue::enq_kernel(Kernel& kernel, ivec3 global_size, ivec3 local_size,
                                       const string& name, const CL::Event& events)
{
    size_t offset[] = {0,0,0};
    size_t global[] = {(size_t)global_size.x, (size_t)global_size.y, (size_t)global_size.z};
    size_t local[]  = {(size_t)local_size.x, (size_t)local_size.y, (size_t)local_size.z};
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;        

    cl_int status;
    status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                    3, offset, global, local,
                                    cnt, _event_pad_ptr, &e);

    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}

CL::Event CL::CommandQueue::enq_write_buffer(Buffer& buffer, void* src, size_t len, size_t offset,
                                             const string& name, const CL::Event& events)
{
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;

    cl_int status;

    status = clEnqueueWriteBuffer(_queue, buffer.get(), CL_FALSE, 
                                  offset, len, src,
                                  cnt, _event_pad_ptr, &e);

    //cout << name << endl;
    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}

CL::Event CL::CommandQueue::enq_write_buffer(Buffer& buffer, void* src, size_t len,
                                             const string& name, const CL::Event& events)
{
    return enq_write_buffer(buffer, src, len, 0, name, events);
}

CL::Event CL::CommandQueue::enq_read_buffer(Buffer& buffer, void* src, size_t len, size_t offset,
                                            const string& name, const CL::Event& events)
{
    cl_int status;
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;

    status = clEnqueueReadBuffer(_queue, buffer.get(), CL_FALSE, 
                                 offset, len, src, cnt, _event_pad_ptr, &e);

    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}


CL::Event CL::CommandQueue::enq_read_buffer(Buffer& buffer, void* src, size_t len,
                                            const string& name, const CL::Event& events)
{
    return enq_read_buffer(buffer, src, len, 0, name, events);
}

void CL::CommandQueue::finish()
{
    cl_int status = clFinish(_queue);

    OPENCL_ASSERT(status);
}

void CL::CommandQueue::flush()
{
    cl_int status = clFlush(_queue);

    OPENCL_ASSERT(status);
}

void CL::CommandQueue::wait_for_events(const CL::Event& events)
{
    size_t num_events = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);

    if (num_events == 0) return;

    if (config.do_event_polling()) {
        // Do polling
        clFlush(_queue);
        for (size_t i = 0; i < num_events; ++i) {
            cl_int eventstatus;
            do {
                cl_int status = clGetEventInfo(_event_pad_ptr[i], 
                                               CL_EVENT_COMMAND_EXECUTION_STATUS,
                                               sizeof(cl_int), &eventstatus, NULL);
				
                OPENCL_ASSERT(status);				
            } while (eventstatus != CL_COMPLETE);
        }
    } else {
        cl_int status = clWaitForEvents(num_events, _event_pad_ptr);
        OPENCL_ASSERT(status);
    }

}



CL::Event CL::CommandQueue::enq_GL_acquire(cl_mem mem, const string& name, const CL::Event& events)
{
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;

    cl_int status = clEnqueueAcquireGLObjects(_queue, 1, &mem, cnt, _event_pad_ptr, &e);

    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}

CL::Event CL::CommandQueue::enq_GL_release(cl_mem mem, const string& name, const CL::Event& events)
{
        
    size_t cnt = _parent_device.setup_event_pad(events, _event_pad, _event_pad_ptr);
    cl_event e;

    cl_int status = clEnqueueReleaseGLObjects(_queue, 1, &mem, cnt, _event_pad_ptr, &e);

    OPENCL_ASSERT(status);

    return _parent_device.insert_event(name, _name, e);
}

void* CL::CommandQueue::map_buffer(Buffer& buffer)
{
    cl_int status;

    void* mapped = clEnqueueMapBuffer(_queue, buffer.get(), CL_TRUE, CL_MAP_READ,
                                      0, buffer.get_size(), 0, 0, 0,
                                      &status);
    OPENCL_ASSERT(status);

    return mapped;
}

void CL::CommandQueue::unmap_buffer(Buffer& buffer, void* mapped)
{
    cl_int status = clEnqueueUnmapMemObject(_queue, buffer.get(), mapped, 
                                            0, NULL, NULL);
    OPENCL_ASSERT(status);
}
