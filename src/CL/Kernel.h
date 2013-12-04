#pragma once

#include "common.h"
#include <CL/opencl.h>

#include "Exception.h"
#include "Buffer.h"

namespace CL
{
    class Device;
    
    class Kernel : public noncopyable
    {
        cl_kernel _kernel;
        cl_program _program;

        public:

        Kernel(cl_program program, cl_device_id device, const string& kernelname);
        ~Kernel();

        template<typename T> void set_arg  (cl_uint arg_index, const T& buffer);

        cl_kernel get() { return _kernel; }

        
        template<typename T, typename ... Types> void set_args(cl_uint i, const T& value, Types && ... rest)
        {
            set_arg(i, value);
            set_args(i+1, rest...);
        }

        
        void set_args(cl_uint i) {}

    };

    template<typename T> 
    inline void Kernel::set_arg(cl_uint arg_index, const T& value) 
    {
        cl_int status;

        status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);
        OPENCL_ASSERT(status);
    }

    template<>
    inline void Kernel::set_arg<ImageBuffer>(cl_uint arg_index, const ImageBuffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }

    template<>
    inline void Kernel::set_arg<Buffer>(cl_uint arg_index, const Buffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }
    
}
