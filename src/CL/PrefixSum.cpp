#include "PrefixSum.h"

#include "Kernel.h"
#include "CommandQueue.h"

#define PREFIX_N 128

CL::PrefixSum::PrefixSum(CL::Device& device, size_t max_input_items, const string& use)
    : _device(device)
    , _use(use)
    , _max_input_items(0)
{
    // Compile program and load kernels

    _program.set_constant("N", PREFIX_N);
    _program.compile(device, "prefix_sum.cl");

    _reduce_kernel.reset(_program.get_kernel("reduce"));
    _accumulate_kernel.reset(_program.get_kernel("accumulate"));

    resize(max_input_items);
    
}


CL::PrefixSum::~PrefixSum()
{

}



CL::Event CL::PrefixSum::apply(size_t batch_size, CL::CommandQueue& queue,
                               CL::Buffer& input, CL::Buffer& output, CL::Buffer& total, CL::Event& event)
{
    if (batch_size == 0) return event;

    if (batch_size <= PREFIX_N) {
        return do_reduce(batch_size, queue, input, output, total, event);
    }

    int reduced_size = (batch_size-1)/PREFIX_N+1;
    int level = _buffer_pyramid.size()-1;
    while (level >= 0 && _buffer_pyramid[level].get_size() < (size_t)reduced_size) {
        level--;
    }
    assert(level >= 0);

    Event ready = do_reduce (batch_size, queue, input, output, _buffer_pyramid.at(level), event);

    ready = apply((batch_size-1)/PREFIX_N+1, queue, _buffer_pyramid.at(level), _buffer_pyramid.at(level), total, ready);

    ready = do_accumulate(batch_size, queue, _buffer_pyramid.at(level), output, ready);

    return ready;
}




CL::Event CL::PrefixSum::do_reduce(size_t batch_size, CL::CommandQueue& queue,
                                   CL::Buffer& input, CL::Buffer& output, CL::Buffer& reduced, CL::Event& event)
{
    _reduce_kernel->set_args((cl_int)batch_size, input, output, reduced);

    return queue.enq_kernel(*_reduce_kernel, ((batch_size-1)/PREFIX_N+1)*(PREFIX_N/2), (PREFIX_N/2), 
                            "reduce", event);
}


CL::Event CL::PrefixSum::do_accumulate(size_t batch_size, CL::CommandQueue& queue,
                                       CL::Buffer& reduced, CL::Buffer& accumulated, CL::Event& event)
{
    _accumulate_kernel->set_args((cl_int)batch_size, reduced, accumulated);

    size_t s = std::min<size_t>(64, PREFIX_N);
    
    return queue.enq_kernel(*_accumulate_kernel, ((batch_size-1)/s)*s, s,
                            "accumulate", event);
}


void CL::PrefixSum::resize(size_t new_max_size)
{
    if (_max_input_items >= new_max_size) return;

    _max_input_items = new_max_size;

    _buffer_pyramid.clear();
    
    // Prepare buffer pyramid
    for (int i = (_max_input_items-1)/PREFIX_N+1; i > 1; i = (i-1)/PREFIX_N+1) {
        _buffer_pyramid.emplace_back(_device, i * sizeof(int), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, _use);
    }
}
