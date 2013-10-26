
#ifndef HWTESSRENDERER_H
#define HWTESSRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

#include "Shader.h"
#include "VBO.h"
#include "Texture.h"
#include "BoundNSplit.h"

namespace Reyes
{
        
    class HWTessRenderer : public PatchDrawer
    {
		GL::Shader _shader;
		GL::VBO _vbo;
        size_t _patch_count;

        struct PatchData
        {
            vector<BezierPatch> patches;
            shared_ptr<GL::TextureBuffer> patch_texture;
        };
        
        map<void*, PatchData> _patch_index;

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

    private:

        void draw_patch(const PatchRange& range);
        void flush();
    };
    
}

#endif
