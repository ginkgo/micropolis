#include "Program.h"

#include "Device.h"
#include "Exception.h"
#include "Kernel.h"

#include "Config.h"

#include <fstream>
#include <iomanip>

namespace {
    cl_program compile_program (CL::Device& device, const string& source, const string& filename);
}



CL::Program::Program() :
    _program(0),
    _source_buffer(new std::stringstream())
{        
    *_source_buffer << std::setiosflags(std::ios::fixed) << std::setprecision(10);
}
    

CL::Program::~Program()
{
    if (_program != 0) {
        clReleaseProgram(_program);
    }
}

void CL::Program::set_constant(const string& name, int value)
{
    assert(_source_buffer);
    *_source_buffer << "#define " << name << " " << value << endl;
}

void CL::Program::set_constant(const string& name, size_t value)
{
    assert(_source_buffer);
    *_source_buffer << "#define " << name << " " << value << endl;
}

void CL::Program::set_constant(const string& name, float value)
{
    assert(_source_buffer);
    *_source_buffer << "#define " << name << " " << value << "f" << endl;
}

void CL::Program::set_constant(const string& name, ivec2 value)
{
    assert(_source_buffer);
    *_source_buffer << "#define " << name << " ((int2)(" 
                    << value.x << ", " << value.y << "))" << endl;
}

void CL::Program::set_constant(const string& name, vec4 value)
{
    assert(_source_buffer);
    *_source_buffer << "#define " << name << " ((float4)(" 
                    << value.x << "f, " << value.y << "f, " 
                    << value.z << "f, " << value.w <<  "f))" << endl;
}

void CL::Program::compile(Device& device,  const string& filename)
{
    assert(_program == 0);

	_device = device.get_device();

    *_source_buffer << endl
                    << "# 1 \"" << config.kernel_dir() << "/" << filename << "\"" << endl
                    << read_file(config.kernel_dir() + "/" + filename);
    
    string file_content = _source_buffer->str();

    if (config.dump_kernel_files()) {
        std::ofstream fs(("/tmp/"+filename).c_str());
        fs << file_content << endl;
    }
    
    _program = compile_program(device, file_content, filename);

    delete _source_buffer;
    _source_buffer = 0;
}


CL::Kernel* CL::Program::get_kernel(const string& name) 
{
    return new Kernel(_program, _device, name);
}
 

namespace {
    
    cl_program compile_program (CL::Device& device, const string& source, const string& filename)
    {
        const char* c_content = source.c_str();
        size_t content_size = source.size();

        cl_int status;

        cl_program program = clCreateProgramWithSource(device.get_context(), 1, 
                                                       &c_content, &content_size, &status);
        OPENCL_ASSERT(status);

        cl_device_id dev = device.get_device();

        string flags = "-I. -cl-fast-relaxed-math -cl-std=CL1.2 -cl-mad-enable";
        flags += " -I"+config.kernel_dir();

        
        status = clBuildProgram(program, 1, &dev, flags.c_str(), NULL, NULL);

        if (status != CL_SUCCESS && status != CL_BUILD_PROGRAM_FAILURE) {
            OPENCL_ASSERT(status);
        }

        cl_build_status build_status;
        
        status = clGetProgramBuildInfo(program, dev, 
                                       CL_PROGRAM_BUILD_STATUS,
                                       sizeof(build_status), &build_status, 
                                       NULL);

        OPENCL_ASSERT(status);

        if (config.verbosity_level() > 0 || build_status != CL_BUILD_SUCCESS) {
            size_t size = 16 * 1024;
            char *buffer = new char[size];

            status = clGetProgramBuildInfo(program, dev,
                                           CL_PROGRAM_BUILD_LOG,
                                           size, buffer, NULL);
            OPENCL_ASSERT(status);
            
            cout << "--------------------------------------------------------------------------------" << endl;
            cout << filename << " build log:" << endl;
            cout << buffer << endl;

            delete[] buffer;
        }


        if (build_status != CL_BUILD_SUCCESS) {
            OPENCL_EXCEPTION("Failed to build program '" + filename + "'");
        }

        return program;
    }
    
}
