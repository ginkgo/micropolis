#pragma once

#include "Patch.h"
#include "Texture.h"

namespace Reyes
{

    class PatchesIndex
    {
        

        struct PatchData
        {
            vector<BezierPatch> patches;
            shared_ptr<GL::TextureBuffer> patch_texture;
        };
        
        map<void*, PatchData> _index;

        bool _is_set_up;
        bool _load_as_texture;
        bool _retain_vector;

        
    public:
        

        PatchesIndex();
        ~PatchesIndex();

        void enable_load_texture();
        void enable_retain_vector();

        bool are_patches_loaded(void* handle);
        void load_patches(void* handle, const vector<BezierPatch>& patch_data);
        void delete_patches(void* handle);
        
        const vector<BezierPatch>& get_patch_vector(void* handle);
        GL::TextureBuffer& get_patch_texture(void* handle);

        
    };

}
