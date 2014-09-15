#pragma once

#include "common.h"

#include "CL/OpenCL.h"
#include "CL/PrefixSum.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "PatchRange.h"
#include "PatchType.h"
#include "Projection.h"

namespace Reyes
{

    
    struct Batch
    {
        size_t patch_count;
        PatchType patch_type;
        CL::Buffer& patch_buffer;
        CL::Buffer& patch_ids;
        CL::Buffer& patch_min;
        CL::Buffer& patch_max;
        CL::Event transfer_done;
    };
    
    class BoundNSplitCL
    {

    public:

        virtual ~BoundNSplitCL() {};
        
    public:

        virtual void init(void* patches_handle, const mat4& matrix, const Projection* projection) = 0;
        virtual bool done() = 0;
        virtual void finish() = 0;

        virtual Batch do_bound_n_split(CL::Event& ready) = 0;
    };

    
}
