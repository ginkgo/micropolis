#include "OpenCL.h"
#include "Config.h"

#include "GL/glx.h"

namespace
{
    
    void get_opencl_device(int& platform_index, int& device_index,
                           cl_platform_id &platform, cl_device_id &device)
    {
        cl_int status;

        cl_platform_id platforms[10];
        cl_uint num_platforms;

    
        status = clGetPlatformIDs(10, platforms, &num_platforms);

        OPENCL_ASSERT(status);

        if (platform_index >= int(num_platforms)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL platforms available.");
            } else {
                cerr << "WARNING: No OpenCL platform " << platform_index 
                     << " available. Falling back to platform number 0." << endl;
                platform_index = 0;
            }
        }

        cl_device_id devices[10];
        cl_uint num_devices;

        status = clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_ALL, 
                                 10, devices, &num_devices);

        OPENCL_ASSERT(status);

        if (device_index >= int(num_devices)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL devices available on platform.");
            } else {
                cerr << "WARNING: No OpenCL device " << device_index 
                     << " available. Falling back to device number 0." << endl;
                device_index = 0;
            }        
        }

        device = devices[device_index];
        platform = platforms[platform_index];
    }

    cl_context create_context(cl_platform_id platform, cl_device_id device)
    {
        cl_int status;

        cl_context_properties props[] = 
            {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 
             CL_GL_CONTEXT_KHR,   (cl_context_properties)glXGetCurrentContext(),
             CL_GLX_DISPLAY_KHR,  (cl_context_properties)XOpenDisplay(":0.0"),
             0};
        cl_context context = clCreateContext(props, 
                                             1, &device,
                                             NULL, NULL, &status);

        OPENCL_ASSERT(status);

        return context;
    }
}

namespace OpenCL
{
    Device::Device(int platform_index, int device_index) 
    {
        cl_platform_id platform;

        get_opencl_device(platform_index, device_index, platform, _device);

        _context = create_context(platform, _device);
    }

    Device::~Device()
    {
        clReleaseContext(_context);
    }

    CommandQueue::CommandQueue(Device& device)
    {
        cl_int status;
        _queue = clCreateCommandQueue(device.get_context(), device.get_device(),
                                      CL_QUEUE_PROFILING_ENABLE, &status);

        OPENCL_ASSERT(status);
    }

    CommandQueue::~CommandQueue()
    {
        clReleaseCommandQueue(_queue);
    }


    void CommandQueue::enq_kernel(Kernel& kernel,
                                  ivec2 global_size, ivec2 local_size)
    {
        size_t offset[] = {0,0};
        size_t global[] = {global_size.x,global_size.y};
        size_t local[]  = {local_size.x, local_size.y};
        

        cl_int status;
        status = clEnqueueNDRangeKernel(_queue, kernel.get(),
                                         2, offset, global, local,
                                         0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::finish()
    {
        cl_int status = clFinish(_queue);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_acquire(ImageBuffer& buffer)
    {
        cl_mem mem = buffer.get();
        cl_int status = clEnqueueAcquireGLObjects(_queue, 1, &mem, 
                                                  0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    void CommandQueue::enq_GL_release(ImageBuffer& buffer)
    {
        cl_mem mem = buffer.get();
        cl_int status = clEnqueueReleaseGLObjects(_queue, 1, &mem, 
                                                  0, NULL, NULL);

        OPENCL_ASSERT(status);
    }

    ImageBuffer::ImageBuffer(Device& device, Texture& texture, cl_mem_flags flags)
    {
        cl_int status;

        _buffer = clCreateFromGLTexture2D(device.get_context(), flags,
                                          GL_TEXTURE_2D, 0, 
                                          texture.texture_name(),
                                          &status);

        OPENCL_ASSERT(status);
    }

    ImageBuffer::~ImageBuffer()
    {
        clReleaseMemObject(_buffer);
    }

    Kernel::Kernel(Device& device, 
                   const string& filename, const string& kernelname)
    {
        string file = config.kernel_dir()+"/"+filename;

        if (!file_exists(file)) {
            OPENCL_EXCEPTION("Failed to load OpenCL kernel program '" + file + "'."); 
        }

        string file_content = read_file(file);
        
        const char* c_content = file_content.c_str();
        size_t content_size = file_content.size();

        cl_int status;

        _program = clCreateProgramWithSource(device.get_context(), 1, 
                                             &c_content, &content_size,
                                             &status);
        OPENCL_ASSERT(status);

        cl_device_id dev = device.get_device();

        status = clBuildProgram(_program, 1, &dev, "", NULL, NULL);

        if (status != CL_SUCCESS && status != CL_BUILD_PROGRAM_FAILURE) {
            OPENCL_ASSERT(status);
        }

        cl_build_status build_status;
        
        status = clGetProgramBuildInfo(_program, dev, 
                                       CL_PROGRAM_BUILD_STATUS,
                                       sizeof(build_status), &build_status, 
                                       NULL);

        OPENCL_ASSERT(status);

        if (build_status != CL_BUILD_SUCCESS) {
            size_t size = 16 * 1024;
            char *buffer = new char[size];

            status = clGetProgramBuildInfo(_program, dev,
                                           CL_PROGRAM_BUILD_LOG,
                                           size, buffer, NULL);
            OPENCL_ASSERT(status);

            cerr << buffer << endl;

            delete[] buffer;
            
            OPENCL_EXCEPTION("Failed to build program '" + file + "'");
        }



        _kernel = clCreateKernel(_program, kernelname.c_str(), &status);
        OPENCL_ASSERT(status);        
    }

    Kernel::~Kernel()
    {
        clReleaseKernel(_kernel);
        clReleaseProgram(_program);
    }


    Exception::Exception(cl_int err_code, const string& file, int line_no):
        _file(file), _line_no(line_no) 
    {
        switch(err_code) {
        case CL_INVALID_PROGRAM: 
            _msg = "Invalid program"; break;
        case CL_INVALID_PROGRAM_EXECUTABLE: 
            _msg = "Program not built successfully"; break;
        case CL_INVALID_KERNEL_NAME: 
            _msg = "Kernel name not found in program"; break;
        case CL_INVALID_KERNEL_DEFINITION:
            _msg = "Invalid kernel definition"; break;
        case CL_INVALID_VALUE:
            _msg = "Invalid value"; break;
        case CL_OUT_OF_RESOURCES:
            _msg = "Out of resources"; break;
        case CL_OUT_OF_HOST_MEMORY:
            _msg = "Out of host memory"; break;
        case CL_INVALID_CONTEXT:
            _msg = "Invalid context"; break;
        case CL_INVALID_MIP_LEVEL:
            _msg = "Invalid miplevel"; break;
        case CL_INVALID_GL_OBJECT:
            _msg = "Invalid OpenGL object"; break;
        case CL_INVALID_OPERATION:
            _msg = "Invalid operation"; break;
        case CL_INVALID_PROPERTY:
            _msg = "Invalid property"; break;
        case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
            _msg = "Invalid OpenGL sharegroup reference"; break;
        case CL_INVALID_DEVICE:
            _msg = "Invalid device"; break;
        case CL_INVALID_BINARY:
            _msg = "Invalid binary"; break;
        case CL_INVALID_BUILD_OPTIONS:
            _msg = "Invalid build options"; break;
        case CL_COMPILER_NOT_AVAILABLE:
            _msg = "Compiler not available"; break;
        case CL_BUILD_PROGRAM_FAILURE:
            _msg = "Program build failure"; break;
        default:
            _msg = "Unknown error";
        }
    }

    Exception::Exception(const string& msg, const string& file, int line_no):
        _msg(msg), _file(file), _line_no(line_no) 
    {

    }

    const string& Exception::msg()
    {
        return _msg;
    }
}


