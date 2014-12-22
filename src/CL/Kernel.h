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
        cl_device_id _device;
        
        public:

        Kernel(cl_program program, cl_device_id device, const string& kernelname);
        ~Kernel();

        template<typename T> void set_arg  (cl_uint arg_index, const T& buffer);

        cl_kernel get() { return _kernel; }


        template<typename ... Types> void set_args(Types && ... values)
        {
            set_args_from(0, values...);
        }
        
        template<typename T, typename ... Types> void set_args_from(cl_uint i, const T& value, Types && ... rest)
        {
            set_arg(i, value);
            set_args_from(i+1, rest...);
        }

        
        void set_args_from(cl_uint i) {}
        
        template <typename T> void get_work_group_info(cl_kernel_work_group_info param_name, T& result)
        {
            cl_int status = clGetKernelWorkGroupInfo(_kernel, _device, param_name,
                                                     sizeof(T), &result, nullptr);
            OPENCL_ASSERT(status);
        }

    };

    template<typename T> 
    inline void Kernel::set_arg(cl_uint arg_index, const T& value) 
    {
        cl_int status = clSetKernelArg(_kernel, arg_index, sizeof(T), &value);

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

    template<>
    inline void Kernel::set_arg<TransferBuffer>(cl_uint arg_index, const TransferBuffer& value)
    {
        cl_mem mem = value.get();
        set_arg(arg_index, mem);
    }
    
}
