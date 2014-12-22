#pragma once


#include "common.h"


#include "BoundNSplitCL.h"


namespace Reyes
{

    
    class PatchIndex;    


    class BoundNSplitCLBreadth : public BoundNSplitCL
    {
        struct PatchBuffer
        {
            CL::Buffer pids;
            CL::Buffer depths;
            CL::Buffer mins;
            CL::Buffer maxs;

            size_t size;
            
            PatchBuffer(CL::Device& device);

            void grow_to(size_t new_size);
        };

        struct FlagBuffer
        {

            CL::Buffer bound_flags;
            CL::Buffer draw_flags;
            CL::Buffer split_flags;
            
            size_t size;
            
            FlagBuffer(CL::Device& device);

            void grow_to(size_t new_size);
        };

        
        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        mat4 _active_matrix;
        PatchType _active_patch_type;
        
        int _patch_count;
            
        CL::CommandQueue& _queue;
        shared_ptr<PatchIndex> _patch_index;
        
        shared_ptr<PatchBuffer> _read_buffers;
        shared_ptr<PatchBuffer> _write_buffers;

        FlagBuffer _flag_buffers;
        CL::PrefixSum _prefix_sum;
        CL::TransferBuffer _split_ranges_cnt_buffer;
        
        CL::Buffer _out_pids_buffer;
        CL::Buffer _out_mins_buffer;
        CL::Buffer _out_maxs_buffer;
        CL::TransferBuffer _out_range_cnt_buffer;

        CL::Buffer _projection_buffer;

        CL::Event _ready;

        CL::UserEvent _user_event;

        CL::Program _bound_n_split_program_bezier;
        CL::Program _bound_n_split_program_gregory;
        
        shared_ptr<CL::Kernel> _bound_kernel_bezier;
        shared_ptr<CL::Kernel> _bound_kernel_gregory;
        shared_ptr<CL::Kernel> _move_kernel;
        shared_ptr<CL::Kernel> _init_ranges_kernel;
        shared_ptr<CL::Kernel> _init_projection_buffer_kernel;
        
    public:

        
        BoundNSplitCLBreadth(CL::Device& device, CL::CommandQueue& queue,
                             shared_ptr<PatchIndex>& patch_index);
        

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);
        
    };
    

}
