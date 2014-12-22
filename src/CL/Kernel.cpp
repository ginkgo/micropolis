#include "Kernel.h"

#include "Exception.h"
#include "Device.h"

#include "Config.h"

CL::Kernel::Kernel(cl_program program, cl_device_id device, const string& kernelname)
    : _program(0)
    , _device(device)
    
{
    cl_int status;

    _kernel = clCreateKernel(program, kernelname.c_str(), &status);
    OPENCL_ASSERT(status);   

	if (config.verbosity_level() > 0) {
	    // Get kernel info

	    size_t work_group_size;
	    size_t preferred_work_group_size_multiple;
	    cl_ulong local_mem_size;
	    cl_ulong private_mem_size;
	    size_t compile_work_group_size[3];

	    status = clGetKernelWorkGroupInfo(_kernel, device, CL_KERNEL_WORK_GROUP_SIZE, 
                                          sizeof(work_group_size), &work_group_size,
                                          NULL);
	    OPENCL_ASSERT(status);

	    status = clGetKernelWorkGroupInfo(_kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, 
                                          sizeof(preferred_work_group_size_multiple), &preferred_work_group_size_multiple,
                                          NULL);
	    OPENCL_ASSERT(status);

	    status = clGetKernelWorkGroupInfo(_kernel, device, CL_KERNEL_LOCAL_MEM_SIZE, 
                                          sizeof(local_mem_size), &local_mem_size,
                                          NULL);
	    OPENCL_ASSERT(status);

	    status = clGetKernelWorkGroupInfo(_kernel, device, CL_KERNEL_PRIVATE_MEM_SIZE, 
                                          sizeof(private_mem_size), &private_mem_size,
                                          NULL);
	    OPENCL_ASSERT(status);

	    status = clGetKernelWorkGroupInfo(_kernel, device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 
                                          sizeof(compile_work_group_size), &compile_work_group_size,
                                          NULL);
	    OPENCL_ASSERT(status);

	    cout << "kernel " << kernelname << ": "
             << local_mem_size << "bytes local, "
             << private_mem_size << "bytes private" << endl;
	}
}

CL::Kernel::~Kernel()
{
    clReleaseKernel(_kernel);
        
    if (_program != 0) {
        clReleaseProgram(_program);
    }
}
