#pragma once

#include "common.h"

#include "Buffer.h"
#include "ComputeShader.h" 

namespace GL
{

    class PrefixSum : public noncopyable
    {
        vector<Buffer> buffer_pyramid;

        ComputeShader reduce;
        ComputeShader accumulate;

        size_t max_input_items;
        
    public:

        PrefixSum(size_t max_input_items);
        ~PrefixSum();

        void apply(size_t batch_size, Buffer& input, Buffer& output, Buffer& total);


    private:

        void do_reduce (size_t batch_size, Buffer& input, Buffer& output, Buffer& reduced);
        void do_accumulate (size_t batch_size, Buffer& reduced, Buffer& accumulated);
    };
}
