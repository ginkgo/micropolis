#pragma once

#include "common.h"
#include <CL/opencl.h>

#include <sstream>

namespace CL
{
    class Kernel;
    class Device;

    class Program : public noncopyable
    {
        cl_device_id _device;
        cl_program _program;
        std::stringstream* _source_buffer;

        public:

        Program();
        ~Program();

        void set_constant(const string& name, int value);
        void set_constant(const string& name, size_t value);
        void set_constant(const string& name, float value);
        void set_constant(const string& name, ivec2 value);
        void set_constant(const string& name, vec4 value);

        void compile(Device& device, const string& filename);

        Kernel* get_kernel(const string& name);
    };
    
}
