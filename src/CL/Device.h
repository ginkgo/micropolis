#pragma once

#include "common.h"
#include <CL/opencl.h>


namespace CL
{
    class Device : public noncopyable
    {
        cl_context   _context;
        cl_device_id _device;

        bool _share_gl;

        public:
        
        Device(int platform_index, int device_index);
        ~Device();

        cl_device_id get_device()  { return _device; }
        cl_context   get_context() { return _context; }

        bool share_gl() const { return _share_gl; }

        void print_info();
    };
}
