#include "ProgramObject.h"

#include "Device.h"
#include "Exception.h"
#include "Kernel.h"

#include "Config.h"
#include "CLConfig.h"

#include <fstream>
#include <iomanip>

#ifdef linux
#include <glob.h>
#else
#error TODO
#endif

namespace {
    cl_program compile_program (CL::Device& device, const string& source, const string& filename);
}



CL::ProgramObject::ProgramObject(const string& filename)
    : _source_file(filename)
{        
    _source_buffer << std::setiosflags(std::ios::fixed) << std::setprecision(10);
}
    

CL::ProgramObject::~ProgramObject()
{
}

void CL::ProgramObject::define(const string& macro, const string& statement)
{
    _source_buffer << "#define " << macro << " " << statement << endl;
}

void CL::ProgramObject::set_constant(const string& name, int value)
{
    _source_buffer << "#define " << name << " " << value << endl;
}

void CL::ProgramObject::set_constant(const string& name, size_t value)
{
    _source_buffer << "#define " << name << " " << value << endl;
}

void CL::ProgramObject::set_constant(const string& name, float value)
{
    _source_buffer << "#define " << name << " " << value << "f" << endl;
}

void CL::ProgramObject::set_constant(const string& name, ivec2 value)
{
    _source_buffer << "#define " << name << " ((int2)(" 
                   << value.x << ", " << value.y << "))" << endl;
}

void CL::ProgramObject::set_constant(const string& name, vec4 value)
{
    _source_buffer << "#define " << name << " ((float4)(" 
                   << value.x << "f, " << value.y << "f, " 
                   << value.z << "f, " << value.w <<  "f))" << endl;
}

cl_program CL::ProgramObject::compile(Device& device) const
{
    std::stringstream ss;

    // Copy stringstream contents over
    ss << _source_buffer.str() << endl;

    // Append filename mark
    ss << "# 1 \"" << cl_config.kernel_dir() << "/" << _source_file << "\"" << endl;

    // Append source file contents
    ss << read_file(cl_config.kernel_dir() + "/" + _source_file);
    
    string file_content = ss.str();

    if (cl_config.dump_kernel_files()) {
        std::ofstream fs(("/tmp/"+_source_file).c_str());
        fs << file_content << endl;
    }

    return compile_program(device, file_content, _source_file);
}
 

namespace {

    void find_headers(CL::Device& device, vector<cl_program>& headers, vector<const char*>& header_names, vector<string>& strings)
    {
        string glob_pattern = cl_config.kernel_dir()+"/*.h";
        glob_t query;
        query.gl_pathc = 0;
        int r;
        if ((r = glob(glob_pattern.c_str(), GLOB_NOSORT, nullptr, &query))&&r != GLOB_NOMATCH) {
            switch (r) {
            case GLOB_NOSPACE:
                cerr << "Kernel header search failed(GLOB_NOSPACE)." << endl;
                break;
            case GLOB_ABORTED:
                cerr << "Kernel header search failed(GLOB_ABORTED)." << endl;
                break;
            }
            
            OPENCL_EXCEPTION("Globbing for kernel headers failed");
        }

        headers.clear();
        header_names.clear();
        strings.clear();

        size_t off = glob_pattern.size()-3;
        
        for (size_t i = 0; i < query.gl_pathc; ++i) {
            strings.push_back(string(query.gl_pathv[i]+off));

            cl_int status;
            
            string contents = read_file(string(query.gl_pathv[i]));
            const char* c_contents = contents.c_str();
            size_t contents_size = contents.size();
            
            headers.push_back(clCreateProgramWithSource(device.get_context(), 1, &c_contents, &contents_size, &status));

            OPENCL_ASSERT(status);
        }
        
        globfree(&query);

        for (size_t i = 0; i < strings.size(); i++) {
            header_names.push_back(strings[i].c_str());
        }        
    }
    
    cl_program compile_program (CL::Device& device, const string& source, const string& filename)
    {
        const char* c_content = source.c_str();
        size_t content_size = source.size();

        cl_int status;

        vector<cl_program> headers;
        vector<const char*> header_names;
        vector<string> header_strings;
        
        find_headers(device, headers, header_names, header_strings);

        cl_program program = clCreateProgramWithSource(device.get_context(), 1, &c_content, &content_size, &status);
        OPENCL_ASSERT(status);

        string flags = "-cl-fast-relaxed-math -cl-std=CL2.0 -cl-mad-enable";
        
#ifdef DEBUG_OPENCL
        flags += " -g";
#endif
        
        cl_device_id dev = device.get_device();
        cl_context context = device.get_context();

        cl_build_status build_status;
       
        status = clCompileProgram(program, 1, &dev, flags.c_str(),
                                  headers.size(), headers.data(), header_names.data(),
                                  nullptr, nullptr);
        
        if (status != CL_SUCCESS && status != CL_COMPILE_PROGRAM_FAILURE) {
            OPENCL_ASSERT(status);
        }

        status = clGetProgramBuildInfo(program, dev, 
                                       CL_PROGRAM_BUILD_STATUS,
                                       sizeof(build_status), &build_status, 
                                       NULL);

        OPENCL_ASSERT(status);

        if (build_status != CL_BUILD_SUCCESS) {
            size_t size = 16 * 1024;
            char *buffer = new char[size];

            status = clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, size, buffer, NULL);
            OPENCL_ASSERT(status);

            cout << "--------------------------------------------------------------------------------" << endl;
            cout << filename << " compiler log:" << endl;
            cout << buffer << endl;

            delete[] buffer;
        }

        return program;
    }
    
}
