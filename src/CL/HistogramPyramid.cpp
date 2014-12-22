#include "HistogramPyramid.h"

#include "Kernel.h"
#include "CommandQueue.h"
#include "ProgramObject.h"

CL::HistogramPyramid::HistogramPyramid(CL::Device& device, const string& use)
    : _device(device)
    , _use(use)
    , _program(device, "histogram_pyramid")
{
    // Compile program and load kernels
    _program.compile("histogram_pyramid.cl");

    _reduce_kernel.reset(_program.get_kernel("reduce"));
}


CL::HistogramPyramid::~HistogramPyramid()
{

}


CL::Event CL::HistogramPyramid::apply(size_t base_item_count, size_t base_size, CommandQueue& queue, Buffer& pyramid, Event& precondition) const
{
    CL::Event event = precondition;

    size_t offset = 0;
    size_t count = base_item_count;
    size_t size = base_size;

    while (count > 1) {
        _reduce_kernel->set_args((cl_int)count, (cl_int)offset, (cl_int)(offset+size), pyramid);
        //cout << "REDUCE: " << count << " " << offset;
        offset += size;
        size = (size+1)/2;
        count = (count+1)/2;

        event = queue.enq_kernel(*_reduce_kernel, round_up_by<int>(count,64), (int)64, "reduce", event);
        //cout << " " << round_up_by<int>(count,64) <<" " << count << " " << offset << " " << endl;
    } 
    
    return event;
}


size_t CL::HistogramPyramid::pyramid_size(size_t base_size) const
{
    size_t sum  = 0;
    size_t size = base_size;

    while (size > 1) {
        sum += size;
        size = (size+1)/2;
    }

    sum += size;
    
    return sum;
}

size_t CL::HistogramPyramid::get_top_level(size_t base_item_count) const
{
    size_t size = base_item_count;
    size_t level = 0;

    while (size > 1) {
        level++;
        size = (size+1)/2;
    }

    return level;
}

void CL::HistogramPyramid::get_offsets(size_t base_size, std::vector<size_t>& offsets) const
{
    size_t sum  = 0;
    size_t size = base_size;

    offsets.clear();
    offsets.push_back(sum);
    
    do {
        sum += size;
        offsets.push_back(sum);
        size = (size+1)/2;
    } while (size > 1);
}

void CL::HistogramPyramid::get_sizes(size_t base_item_count, std::vector<size_t>& sizes) const
{
    size_t size = base_item_count;
    size_t level = 0;

    sizes.clear();
    sizes.push_back(size);
    
    while (size > 1) {
        level++;
        size = (size+1)/2;
        sizes.push_back(size);
    }
}
