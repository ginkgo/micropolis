#pragma once


#include "common.h"


#include "BoundNSplitCL.h"


namespace Reyes
{

    
    class PatchIndex;    


    class BoundNSplitCLMultipass : public BoundNSplitCL
    {

        CL::CommandQueue& _queue;
        shared_ptr<PatchIndex> _patch_index;
        
        CL::Program _bound_n_split_program;

        shared_ptr<CL::Kernel> _bound_kernel;
        shared_ptr<CL::Kernel> _move_kernel;
        shared_ptr<CL::Kernel> _init_ranges_kernel;
        shared_ptr<CL::Kernel> _init_projection_buffer_kernel;
        
        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        mat4 _active_matrix;
        int _stack_height;
        
        CL::Buffer _pid_stack;
        CL::Buffer _depth_stack;
        CL::Buffer _min_stack;
        CL::Buffer _max_stack;

        CL::TransferBuffer _split_ranges_cnt_buffer;
        
        CL::Buffer _out_pids_buffer;
        CL::Buffer _out_mins_buffer;
        CL::Buffer _out_maxs_buffer;
        CL::TransferBuffer _out_range_cnt_buffer;

        CL::Buffer _bound_flags;
        CL::Buffer _draw_flags;
        CL::Buffer _split_flags;
        
        CL::Buffer _pid_pad;
        CL::Buffer _depth_pad;
        CL::Buffer _min_pad;
        CL::Buffer _max_pad;

        CL::Buffer _projection_buffer;

        CL::Event _ready;

        CL::PrefixSum _prefix_sum;
        
    public:

        
        BoundNSplitCLMultipass(CL::Device& device, CL::CommandQueue& queue,
                               shared_ptr<PatchIndex>& patch_index);
        

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);
        
    };
    

}
