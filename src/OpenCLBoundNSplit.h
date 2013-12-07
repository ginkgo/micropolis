#pragma once

#include "common.h"

#include "BoundNSplit.h"
#include "CL/OpenCL.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "Projection.h"

namespace Reyes
{

    struct Batch
    {
        size_t patch_count;
        CL::Buffer& patch_buffer;
        CL::Buffer& patch_ids;
        CL::Buffer& patch_min;
        CL::Buffer& patch_max;
        CL::Event transfer_done;
    };
    
    class PatchIndex;    

    class OpenCLBoundNSplit
    {

        CL::CommandQueue& _queue;
        CL::Program _bound_n_split_program;
        
        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;

        
    public:

        
        OpenCLBoundNSplit(CL::Device& device, CL::CommandQueue& queue, shared_ptr<PatchIndex>& patch_index);

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();
        void finish();

        Batch do_bound_n_split(CL::Event& ready);

        
    private:

        
        CL::UserEvent _bound_n_split_event;

        enum BatchStatus {
            INACTIVE,    // Currently unused
            SET_UP,      // Data set up and transfer queued
            ACCEPTED     // Picked up by rasterization stages
        };
                
        
        struct BatchRecord 
        {
            BatchStatus status;
            
            CL::TransferBuffer patch_ids;
            CL::TransferBuffer patch_min;
            CL::TransferBuffer patch_max;
            
            CL::Event transferred;
            CL::Event rasterizer_done;

            BatchRecord(size_t batch_size, CL::Device& device, CL::CommandQueue& queue);

            BatchRecord(BatchRecord&& other);         
            BatchRecord& operator=(BatchRecord&& other);
            
            void transfer(CL::CommandQueue& queue, size_t patch_count);
            void accept(CL::Event& event);
            void finish(CL::CommandQueue& queue);
        };

        vector<BatchRecord> _batch_records;
        size_t _next_batch_record;
        
    };
}
