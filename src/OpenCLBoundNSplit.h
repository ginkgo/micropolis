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
        
        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;
        CL::Buffer* _active_patch_buffer;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;


        CL::TransferBuffer _patch_ids;
        CL::TransferBuffer _patch_min;
        CL::TransferBuffer _patch_max;
        
    public:

        
        OpenCLBoundNSplit(CL::Device& device, CL::CommandQueue& queue, shared_ptr<PatchIndex>& patch_index);

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        Batch do_bound_n_split(CL::Event& ready);
        
    };
}
