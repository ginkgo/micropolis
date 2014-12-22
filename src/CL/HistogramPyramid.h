#pragma once

#include "common.h"

#include "Buffer.h"
#include "Event.h"
#include "Program.h"
#include "CommandQueue.h"


namespace CL
{
    class Kernel;

    class HistogramPyramid : public noncopyable
    {

        Device& _device;
        string _use;
     
        Program _program;
        
        shared_ptr<Kernel> _reduce_kernel;
        
    public:

        HistogramPyramid(Device& device, const string& use="unknown");
        ~HistogramPyramid();

        Event apply(size_t base_item_count, size_t base_size, CommandQueue& queue, Buffer& pyramid, Event& event) const;

        size_t pyramid_size(size_t base_size) const;
        size_t get_top_level(size_t base_item_count) const;
        
        void get_offsets(size_t base_size, std::vector<size_t>& offsets) const;
        void get_sizes(size_t base_item_count, std::vector<size_t>& sizes) const;
        
    };


}
