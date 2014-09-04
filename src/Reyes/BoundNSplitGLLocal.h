#pragma once

#include "common.h"

#include "BoundNSplitGL.h"


namespace Reyes
{

    class PatchIndex;


    
    class BoundNSplitGLLocal : public BoundNSplitGL
    {

        shared_ptr<PatchIndex> _patch_index;

        GL::ComputeShader _bound_n_split_kernel;
        GL::ComputeShader _clear_out_range_cnt_kernel;
        GL::ComputeShader _init_count_buffers_kernel;
        GL::ComputeShader _init_range_buffers_kernel;
        GL::ComputeShader _setup_indirection_kernel;

        void* _active_handle;
        mat4 _active_matrix;

        size_t _in_buffer_size;
        size_t _in_buffer_stride;

        GL::Buffer _in_pids_buffer;
        GL::Buffer _in_depths_buffer;
        GL::Buffer _in_mins_buffer;
        GL::Buffer _in_maxs_buffer;
        GL::Buffer _in_range_cnt_buffer;

        GL::Buffer _out_range_cnt_buffer;

        bool _done;
        int _iteration_count;
        
    public:

        BoundNSplitGLLocal(shared_ptr<PatchIndex>& patch_index);
        ~BoundNSplitGLLocal();

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        void do_bound_n_split(GL::IndirectVBO& vbo);
        
        
    };

}
