#include "Buffer.h"

#include "CommandQueue.h"
#include "Device.h"
#include "Exception.h"

#include "Statistics.h"

#include <CL/cl_gl.h>


CL::Buffer::Buffer(Device& device, size_t size, cl_mem_flags flags)
{
    cl_int status;
    _buffer = clCreateBuffer(device.get_context(), flags, size, NULL,
                             &status);

    if (status == CL_INVALID_BUFFER_SIZE) {
        OPENCL_EXCEPTION("Invalid buffer size: " + to_string(size) + " bytes");                            
    }

    OPENCL_ASSERT(status);

    statistics.alloc_opencl_memory(get_size());
}

CL::Buffer::Buffer(Device& device, CommandQueue& queue, size_t size, cl_mem_flags flags, void** host_ptr)
{
    cl_int status;
    _buffer = clCreateBuffer(device.get_context(), 
                             flags | CL_MEM_ALLOC_HOST_PTR, 
                             size, NULL, &status);

    OPENCL_ASSERT(status);

    *host_ptr = queue.map_buffer(*this);
    queue.unmap_buffer(*this, *host_ptr);

    assert(*host_ptr != NULL);

    statistics.alloc_opencl_memory(get_size());
}

CL::Buffer::Buffer(Device& device, size_t size, cl_mem_flags flags, void* host_ptr)
{
    cl_int status;
    _buffer = clCreateBuffer(device.get_context(), 
                             flags | CL_MEM_USE_HOST_PTR, 
                             size, host_ptr, &status);

    OPENCL_ASSERT(status);

    statistics.alloc_opencl_memory(get_size());
}

CL::Buffer::Buffer(Device& device, GLuint GL_buffer) 
{
    cl_int status;

    _buffer = clCreateFromGLBuffer(device.get_context(), 
                                   CL_MEM_WRITE_ONLY, GL_buffer, &status);

    OPENCL_ASSERT(status);

    statistics.alloc_opencl_memory(get_size());
}

CL::Buffer::~Buffer()
{
    if (_buffer != 0) {
        statistics.free_opencl_memory(get_size());
        clReleaseMemObject(_buffer);
    }
}

size_t CL::Buffer::get_size() const
{
    size_t size;
    cl_int status = clGetMemObjectInfo(_buffer, CL_MEM_SIZE, sizeof(size), &size, NULL);

    OPENCL_ASSERT(status);

    return size;
}


CL::TransferBuffer::TransferBuffer(Device& device, CL::CommandQueue& queue, size_t size, cl_mem_flags flags)
    //: CL::Buffer(device, queue, size, flags, &_host_ptr)
    : CL::Buffer(device, size, flags)
    , _host_ptr(malloc(size))
{
}

CL::TransferBuffer::~TransferBuffer()
{
    if (_host_ptr != nullptr) {
        free(_host_ptr);
    }
}


CL::TransferBuffer::TransferBuffer(TransferBuffer&& other)
{
    _buffer = std::move(other._buffer);
    _host_ptr = std::move(other._host_ptr);
}


CL::TransferBuffer& CL::TransferBuffer::operator = (TransferBuffer&& other)
{
    _buffer = std::move(other._buffer);
    _host_ptr = std::move(other._host_ptr);

    other._buffer = 0;
    other._host_ptr = nullptr;
    
    return *this;
}


void* CL::TransferBuffer::host_ptr()
{
    return _host_ptr;
}



CL::ImageBuffer::ImageBuffer(Device& device, GL::Texture& texture, cl_mem_flags flags)
{
    cl_int status;

    _buffer = clCreateFromGLTexture(device.get_context(), flags,
                                    GL_TEXTURE_2D, 0, 
                                    texture.texture_name(),
                                    &status);

    OPENCL_ASSERT(status);
}

CL::ImageBuffer::~ImageBuffer()
{
    clReleaseMemObject(_buffer);
}
