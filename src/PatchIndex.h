#pragma once

#include "Patch.h"
#include "GL/Texture.h"
#include "CL/OpenCL.h"

namespace Reyes
{

    class PatchIndex
    {
        

        struct PatchData
        {
            size_t patch_count;
            vector<BezierPatch> patches;
            shared_ptr<GL::TextureBuffer> patch_texture;
            shared_ptr<CL::Buffer> opencl_buffer;
        };
        
        map<void*, PatchData> _index;

        bool _is_set_up;
        bool _load_as_texture;
        bool _load_as_opencl_buffer;
        bool _retain_vector;

        CL::Device* _opencl_device;
        CL::CommandQueue* _opencl_queue;
        
    public:
        

        PatchIndex();
        ~PatchIndex();

        void enable_load_texture();
        void enable_load_opencl_buffer(CL::Device& opencl_device, CL::CommandQueue& opencl_queue);
        void enable_retain_vector();

        bool are_patches_loaded(void* handle);
        void load_patches(void* handle, const vector<BezierPatch>& patch_data);
        void delete_patches(void* handle);
        
        const vector<BezierPatch>& get_patch_vector(void* handle);
        GL::TextureBuffer& get_patch_texture(void* handle);
        CL::Buffer* get_opencl_buffer(void* handle);
        
        size_t get_patch_count(void* handle);

        
    };

}
