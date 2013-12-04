#pragma once

#include "common.h"
#include <CL/opencl.h>


namespace CL
{

    class Exception
    {
        string _msg;
        string _file;
        int _line_no;

        public:

        Exception(cl_int err_code,   const string& file, int line_no);
        Exception(const string& msg, const string& file, int line_no);
        
        const string& msg();
        const string& file() { return _file; }
        const int line_no() { return _line_no; }
    };
    
}

#define OPENCL_EXCEPTION(error) throw CL::Exception((error), __FILE__, __LINE__)
#define OPENCL_ASSERT(error) if ((error)!= CL_SUCCESS) throw CL::Exception((error), __FILE__, __LINE__)
