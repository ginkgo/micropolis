#pragma once

#include "common.h"

#include "GL/ComputeShader.h"
#include "GL/PrefixSum.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "PatchRange.h"
#include "Projection.h"

namespace Reyes
{

    
    class BoundNSplitGL
    {
        
    public:

        virtual ~BoundNSplitGL() {};

        virtual void init(void* patches_handle, const mat4& matrix, const Projection* projection) = 0;
        virtual bool done() = 0;

        virtual void do_bound_n_split(GL::IndirectVBO& vbo) = 0;
        
    };


}
