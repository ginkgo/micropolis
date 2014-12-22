#pragma once

#include "common.h"
#include <CL/opencl.h>

#include "ProgramObject.h"

#include <sstream>

namespace CL
{
    class Kernel;
    class Device;

    class Program : public noncopyable
    {
        Device& _device;
        cl_program _program;
        string _name;

        public:

        Program(Device& device, const string& program_name);
        ~Program();

        void compile(const string& source_file);
        void link(const ProgramObject& object);
        void link(const vector<const ProgramObject*>& objects);

        void print_program_info();
        
        Kernel* get_kernel(const string& name);
    };
    
}
