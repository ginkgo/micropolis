#include "PrefixSum.h"


GL::PrefixSum::PrefixSum(size_t max_input_items)
    : reduce("reduce")
    , accumulate("accumulate")
    , max_input_items(max_input_items)
{
    for (int i = (max_input_items-1)/128+1; i > 1; i = (i-1)/128+1) {
        buffer_pyramid.emplace_back(i * sizeof(ivec2));
    }
}


GL::PrefixSum::~PrefixSum()
{
    
}


void GL::PrefixSum::apply(size_t batch_size, GL::Buffer& input, GL::Buffer& output, Buffer& total)
{
    if (batch_size == 0) return;
    
    if (batch_size <= 128) {
        do_reduce(batch_size, input, output, total);
        //do_accumulate(batch_size, total, output);
        return;
    }
    
    int level = 0;

    for (int i = max_input_items; i > batch_size; i = (i-1)/128+1) {
        level++;
    }

    do_reduce (batch_size, input, output, buffer_pyramid[level]);

    apply((batch_size-1)/128+1, buffer_pyramid[level], buffer_pyramid[level], total);

    do_accumulate(batch_size, buffer_pyramid[level], output);
}


void GL::PrefixSum::do_reduce(size_t batch_size, GL::Buffer& input, GL::Buffer& output, GL::Buffer& reduced)
{
    reduce.bind();
    
    if (input.get_id() == output.get_id())
        GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0, input, reduced);
    else
        GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0, input, output, reduced);

    reduce.set_uniform("batch_size", (GLuint)batch_size);
    reduce.set_buffer("input_buffer", input);
    reduce.set_buffer("output_buffer", output);
    reduce.set_buffer("reduced_buffer", reduced);

    reduce.dispatch((batch_size-1)/128+1);
    
    if (input.get_id() == output.get_id())
        GL::Buffer::unbind_all(input, reduced);
    else
        GL::Buffer::unbind_all(input, output, reduced);
    
    reduce.unbind();
}


void GL::PrefixSum::do_accumulate(size_t batch_size, GL::Buffer& reduced, GL::Buffer& accumulated)
{
    if ((batch_size-1)/128 == 0) return;
    
    accumulate.bind();

    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0, reduced, accumulated);

    accumulate.set_uniform("batch_size", (GLuint)batch_size);
    accumulate.set_buffer("reduced", reduced);
    accumulate.set_buffer("accumulated", accumulated);

    accumulate.dispatch((batch_size-1)/128);
    
    GL::Buffer::unbind_all(reduced, accumulated);

    accumulate.unbind();
}
