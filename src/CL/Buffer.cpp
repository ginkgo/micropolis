#include "Buffer.h"

#include "CommandQueue.h"
#include "Device.h"
#include "Exception.h"

#include "Config.h"
#include "Statistics.h"

#include <CL/cl_gl.h>


/*****************************************************************************\
                                    Buffer
\*****************************************************************************/

CL::Buffer::Buffer(Device& device, size_t size, cl_mem_flags flags)
    : _device(&device)
    , _flags(flags)
    , _buffer(0)
{
    resize(size);
}


CL::Buffer::Buffer(Device& device, GLuint GL_buffer)
    : _device(&device)
    , _flags(CL_MEM_WRITE_ONLY)
{
    cl_int status;
    
    _buffer = clCreateFromGLBuffer(_device->get_context(), _flags, GL_buffer, &status);
    OPENCL_ASSERT(status);
    
    size_t size;
    status = clGetMemObjectInfo(_buffer, CL_MEM_SIZE, sizeof(size), &size, NULL);
    OPENCL_ASSERT(status);
    _size = size;
}


CL::Buffer::~Buffer()
{
    if (_buffer != 0) {
        statistics.free_opencl_memory(_size);
        clReleaseMemObject(_buffer);
    }
}



CL::Buffer::Buffer(Buffer&& other)
    : _device(other._device)
    , _flags(other._flags)
    , _size(other._size)
    , _buffer(other._buffer)
{
    other._device = nullptr;
    other._flags = 0;
    other._size = 0;
    other._buffer = 0;
}


CL::Buffer& CL::Buffer::operator=(Buffer&& other)
{
    _device = other._device;
    _flags = other._flags;
    _size = other._size;
    _buffer = other._buffer;
        
    other._device = nullptr;
    other._flags = 0;
    other._size = 0;
    other._buffer = 0;

    return *this;
}



size_t CL::Buffer::get_size() const
{

    return _size;
}


void CL::Buffer::resize(size_t new_size)
{
    cl_int status;
    
    if (_buffer != 0) {
        statistics.free_opencl_memory(get_size());
        clReleaseMemObject(_buffer);

        _buffer = 0;
        _size = 0;
    }
    
    _size = new_size;
    if (new_size == 0) return;
    
    _buffer = clCreateBuffer(_device->get_context(), _flags, new_size, NULL, &status);
    OPENCL_ASSERT(status);

    statistics.alloc_opencl_memory(_size);
}


/*****************************************************************************\
                                  ImageBuffer
\*****************************************************************************/

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


/*****************************************************************************\
                                TransferBuffer
\*****************************************************************************/


CL::TransferBuffer::TransferBuffer(Device& device, size_t size, cl_mem_flags flags)
    : CL::Buffer(device, flags | CL_MEM_ALLOC_HOST_PTR)
    , _host_ptr(nullptr)
{
    resize(size);
}


CL::TransferBuffer::~TransferBuffer()
{

    if (config.transfer_buffer_mode() == Config::UNPINNED && _host_ptr != nullptr) {
        free(_host_ptr);
    }
}




CL::TransferBuffer::TransferBuffer(TransferBuffer&& other)
    : Buffer(*other._device, other._flags)
{
    _buffer = std::move(other._buffer);
    _host_ptr = std::move(other._host_ptr);
    _size = std::move(other._size);
    
    other._buffer = 0;
    other._host_ptr = 0;
    other._size = 0;    
}


CL::TransferBuffer& CL::TransferBuffer::operator = (TransferBuffer&& other)
{
    _device = std::move(other._device);
    _flags = std::move(other._flags);
    
    _buffer = std::move(other._buffer);
    _host_ptr = std::move(other._host_ptr);

    other._device = 0;
    other._buffer = 0;
    other._host_ptr = nullptr;
    
    return *this;
}




void CL::TransferBuffer::resize(size_t new_size)
{
    Buffer::resize(new_size);

    if (config.transfer_buffer_mode() == Config::PINNED) {
        
        cl_map_flags map_flags = 0;

        if (_flags & CL_MEM_HOST_WRITE_ONLY) {
            map_flags = CL_MAP_WRITE;
        } else if (_flags & CL_MEM_HOST_READ_ONLY) {
            map_flags = CL_MAP_READ;
        } else {
            assert(0);
        }
    
        CommandQueue queue(*_device, "map queue");
        queue.map_buffer(*this, map_flags, "initial map", Event());
        
    } else {
        
        if (_host_ptr != nullptr) 
            free(_host_ptr);
        _host_ptr = malloc(new_size);
        
    }
}




void* CL::TransferBuffer::void_ptr()
{
    return _host_ptr;
}

