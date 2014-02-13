#pragma once

#include "common.h"

#include "Buffer.h"
#include "Event.h"
#include "Program.h"
#include "CommandQueue.h"


namespace CL
{
    class Kernel;

    class PrefixSum : public noncopyable
    {
        vector<Buffer> _buffer_pyramid;

        Program _program;
        
        shared_ptr<Kernel> _reduce_kernel;
        shared_ptr<Kernel> _accumulate_kernel;

        Device& _device;
        string _use;
        
        size_t _max_input_items;

    public:

        PrefixSum(Device& device, size_t max_input_items, const string& use="unknown");
        ~PrefixSum();

        Event apply(size_t batch_size, CommandQueue& queue, Buffer& input, Buffer& output, Buffer& total, Event& event);

        void resize(size_t new_max_size);
        
    private:

        Event do_reduce (size_t batch_size, CommandQueue& queue, Buffer& input, Buffer& output, Buffer& reduced, Event& event);
        Event do_accumulate (size_t batch_size, CommandQueue& queue, Buffer& reduced, Buffer& accumulated, Event& event);
    };


}
