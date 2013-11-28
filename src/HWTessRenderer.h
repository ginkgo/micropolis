
#ifndef HWTESSRENDERER_H
#define HWTESSRENDERER_H


#include "BoundNSplit.h"
#include "Patch.h"
#include "PatchDrawer.h"
#include "PatchesIndex.h"
#include "Shader.h"
#include "Texture.h"
#include "VBO.h"


namespace Reyes
{
        
    class HWTessRenderer : public PatchDrawer
    {
		GL::Shader _shader;
		GL::IndirectVBO _vbo;
        
        shared_ptr<PatchesIndex> _patch_index;
        shared_ptr<BoundNSplit> _bound_n_split;

    public:

        HWTessRenderer();
        ~HWTessRenderer() {};

        virtual void prepare();
        virtual void finish();
        
        virtual bool are_patches_loaded(void* patches_handle);
        virtual void load_patches(void* patches_handle, const vector<BezierPatch>& patch_data);
        
        virtual void draw_patches(void* patches_handle,
                                  const mat4& matrix,
                                  const Projection* projection,
                                  const vec4& color);
    };
    
}

#endif
