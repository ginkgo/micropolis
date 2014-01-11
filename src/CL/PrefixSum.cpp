#include "PrefixSum.h"

#include "Kernel.h"
#include "CommandQueue.h"

CL::PrefixSum::PrefixSum(CL::Device& device, size_t max_input_items)
    : _max_input_items(max_input_items)
{
    // Compile program and load kernels
    _program.compile(device, "prefix_sum.cl");

    _reduce_kernel.reset(_program.get_kernel("reduce"));
    _accumulate_kernel.reset(_program.get_kernel("accumulate"));
    
    // Prepare buffer pyramid
    for (int i = (max_input_items-1)/128+1; i > 1; i = (i-1)/128+1) {
        _buffer_pyramid.emplace_back(device, i * sizeof(int), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS);
    }
}


CL::PrefixSum::~PrefixSum()
{

}



CL::Event CL::PrefixSum::apply(size_t batch_size, CL::CommandQueue& queue,
                               CL::Buffer& input, CL::Buffer& output, CL::Buffer& total, CL::Event& event)
{
    if (batch_size == 0) return event;

    if (batch_size <= 128) {
        return do_reduce(batch_size, queue, input, output, total, event);
    }

    int reduced_size = (batch_size-1)/128+1;
    int level = _buffer_pyramid.size()-1;
    while (level >= 0 && _buffer_pyramid[level].get_size() < reduced_size) {
        level--;
    }
    assert(level >= 0);

    Event ready = do_reduce (batch_size, queue, input, output, _buffer_pyramid.at(level), event);

    ready = apply((batch_size-1)/128+1, queue, _buffer_pyramid.at(level), _buffer_pyramid.at(level), total, ready);

    ready = do_accumulate(batch_size, queue, _buffer_pyramid.at(level), output, ready);

    return ready;
}




CL::Event CL::PrefixSum::do_reduce(size_t batch_size, CL::CommandQueue& queue,
                                   CL::Buffer& input, CL::Buffer& output, CL::Buffer& reduced, CL::Event& event)
{
    _reduce_kernel->set_args((cl_int)batch_size, input, output, reduced);

    return queue.enq_kernel(*_reduce_kernel, ((batch_size-1)/128+1)*64, 64,
                            "reduce", event);
}


CL::Event CL::PrefixSum::do_accumulate(size_t batch_size, CL::CommandQueue& queue,
                                       CL::Buffer& reduced, CL::Buffer& accumulated, CL::Event& event)
{
    _accumulate_kernel->set_args((cl_int)batch_size, reduced, accumulated);

    return queue.enq_kernel(*_accumulate_kernel, ((batch_size-1)/128)*128, 128,
                            "accumulate", event);
}

