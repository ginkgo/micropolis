#pragma once

#include "common.h"
#include <CL/opencl.h>

#include <sstream>

namespace CL
{
    class Device;

    class ProgramObject : public noncopyable
    {
        string _source_file;
        std::stringstream _source_buffer;
        
    public:

        ProgramObject(const string& filename);
        ~ProgramObject();
        
        void define(const string& macro, const string& statement);
        
        void set_constant(const string& name, int value);
        void set_constant(const string& name, size_t value);
        void set_constant(const string& name, float value);
        void set_constant(const string& name, ivec2 value);
        void set_constant(const string& name, vec4 value);

        cl_program compile(Device& device) const;

    };
}
