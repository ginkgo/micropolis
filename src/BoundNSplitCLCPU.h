#pragma once

#include "common.h"

#include "BoundNSplitCL.h"


namespace Reyes
{
        

    class PatchIndex;    

    
    class BoundNSplitCLCPU : public BoundNSplitCL
    {

        CL::CommandQueue& _queue;
                
        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;

        
    public:

        
        BoundNSplitCLCPU(CL::Device& device, CL::CommandQueue& queue,
                             shared_ptr<PatchIndex>& patch_index);

        virtual void init(void* patches_handle,
                          const mat4& matrix, const Projection* projection);
        virtual bool done();
        virtual void finish();

        virtual Batch do_bound_n_split(CL::Event& ready);

        
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

            void transfer(CL::CommandQueue& queue, size_t patch_count, const CL::Event& events);
            
            void accept(CL::Event& event);
            CL::Event finish(CL::CommandQueue& queue);
        };

        vector<BatchRecord> _batch_records;
        size_t _next_batch_record;


    private:

        
        static void vsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void hsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void bound_patch_range (const PatchRange& r, const BezierPatch& p,
                                       const mat4& mv, const mat4& mvp,
                                       BBox& box, float& vlen, float& hlen);

        
    };

    
}
