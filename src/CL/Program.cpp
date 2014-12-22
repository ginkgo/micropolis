#include "Program.h"

#include "Device.h"
#include "Exception.h"
#include "Kernel.h"
#include "ProgramObject.h"

#include "Config.h"
#include "CLConfig.h"

#include <fstream>
#include <iomanip>

#ifdef linux
#include <glob.h>
#else
#error TODO
#endif


CL::Program::Program(Device& device, const string& program_name)
    : _device(device)
    , _program(0)
    , _name(program_name)
{   
}
    

CL::Program::~Program()
{
    if (_program != 0) {
        clReleaseProgram(_program);
    }
}


void CL::Program::compile(const string& source_file)
{
    ProgramObject program_object(source_file);
    link(program_object);
}


void CL::Program::link(const ProgramObject& program_object)
{
    vector<const ProgramObject*> objects = {&program_object};

    link(objects);
}

void CL::Program::link(const vector<const CL::ProgramObject*>& objects)
{
    assert(_program == 0);

    if (config.verbosity_level() > 0) {
        cout  << "Linking " << _name << "..." << endl;
    }
    
    cl_int status;
    cl_device_id dev = _device.get_device();
    cl_context context = _device.get_context();

    vector<cl_program> program_objects;

    for (auto o : objects) {
        program_objects.push_back(o->compile(_device));
    }

    string link_flags = "";

    _program = clLinkProgram(context, 1, &dev, link_flags.c_str(),
                             program_objects.size(), program_objects.data(),
                             nullptr, nullptr,
                             &status);
    OPENCL_ASSERT(status);

    // if (status != CL_SUCCESS && status != CL_LINK_PROGRAM_FAILURE) {
    //     OPENCL_ASSERT(status);
    // }
        
    cl_build_status build_status;
    status = clGetProgramBuildInfo(_program, dev, CL_PROGRAM_BUILD_STATUS,
                                   sizeof(build_status), &build_status, 
                                   NULL);
    OPENCL_ASSERT(status);

    if (config.verbosity_level() > 0 || build_status != CL_BUILD_SUCCESS) {
        size_t size = 16 * 1024;
        char *buffer = new char[size];

        status = clGetProgramBuildInfo(_program, dev, CL_PROGRAM_BUILD_LOG, size, buffer, NULL);
        OPENCL_ASSERT(status);

        if (strlen(buffer)>0) {
            cout << "--------------------------------------------------------------------------------" << endl;
            cout << _name << " linker log:" << endl;
            cout << buffer << endl;
        }
            
        delete[] buffer;
    }


    if (build_status != CL_BUILD_SUCCESS) {
        OPENCL_EXCEPTION("Failed to link program '" + _name + "'");
    }

    if (config.verbosity_level() > 0) {
        print_program_info();
    }
    
}


void CL::Program::print_program_info()
{
    if (_program == 0) {
        return;
    }
    
    cl_int status;

    size_t num_kernels;
    
    status = clGetProgramInfo(_program, CL_PROGRAM_NUM_KERNELS,
                              sizeof(num_kernels), &num_kernels,
                              nullptr);
    OPENCL_ASSERT(status);

    cout << num_kernels << " kernels built: ";

    char kernel_names[1024*16];

    status = clGetProgramInfo(_program, CL_PROGRAM_KERNEL_NAMES,
                              sizeof(kernel_names), kernel_names,
                              nullptr);
    OPENCL_ASSERT(status);

    cout << kernel_names << endl;
    
}


CL::Kernel* CL::Program::get_kernel(const string& name) 
{
    assert(_program != 0);
    
    return new Kernel(_program, _device.get_device(), name);
}
