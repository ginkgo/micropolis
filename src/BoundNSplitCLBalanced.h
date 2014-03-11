#pragma once

#include "common.h"

#include "BoundNSplitCL.h"


namespace Reyes
{
        

    class PatchIndex;    

    
    class BoundNSplitCLBalanced : public BoundNSplitCL
    {
        
        CL::CommandQueue& _queue;                
        shared_ptr<PatchIndex> _patch_index;
        
        CL::Program _bound_n_split_program;

        shared_ptr<CL::Kernel> _bound_n_split_kernel;
        shared_ptr<CL::Kernel> _init_range_buffers_kernel;
        shared_ptr<CL::Kernel> _init_projection_buffer_kernel;
        shared_ptr<CL::Kernel> _init_count_buffers_kernel;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        mat4 _active_matrix;

        size_t _in_buffers_size;
        size_t _in_buffer_stride;
        CL::Buffer _in_pids_buffer;
        CL::Buffer _in_mins_buffer;
        CL::Buffer _in_maxs_buffer;
        CL::Buffer _in_range_cnt_buffer;
        
        CL::Buffer _out_pids_buffer;
        CL::Buffer _out_mins_buffer;
        CL::Buffer _out_maxs_buffer;
        CL::TransferBuffer _out_range_cnt_buffer;

        CL::TransferBuffer _processed_count_buffer;

        CL::Buffer _projection_buffer;

        CL::Event _ready;

        bool _done;
        int _iteration_count;
        
    public:


        BoundNSplitCLBalanced(CL::Device& device, CL::CommandQueue& queue,
                              shared_ptr<PatchIndex>& patch_index);
        

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);
    };
    

}
