
#include "PatchIndex.h"

#include "common.h"


Reyes::PatchIndex::PatchIndex()
    : _is_set_up(false)
    , _load_as_texture(false)
    , _load_as_opencl_buffer(false)
    , _retain_vector(false)
    , _opencl_device(nullptr)
    , _opencl_queue(nullptr)
{
        
}


Reyes::PatchIndex::~PatchIndex()
{

}


void Reyes::PatchIndex::enable_load_texture()
{
    assert(!_is_set_up);
    _load_as_texture = true;
}


void Reyes::PatchIndex::enable_load_opencl_buffer(CL::Device& opencl_device, CL::CommandQueue& opencl_queue)
{
    assert(!_is_set_up);

    if (_load_as_texture) return;
    
    _load_as_opencl_buffer = true;
    _opencl_device = &opencl_device;
    _opencl_queue = &opencl_queue;
}


void Reyes::PatchIndex::enable_retain_vector()
{
    assert(!_is_set_up);
    _retain_vector = true;
}


bool Reyes::PatchIndex::are_patches_loaded(void* handle)
{
    return _index.count(handle) > 0;
}


void Reyes::PatchIndex::load_patches(void* handle, const vector<BezierPatch>& patch_data)
{
    assert(handle != nullptr);
    
    PatchData& record = _index[handle];

    record.patch_count = patch_data.size();

    size_t data_size = patch_data.size() * sizeof(BezierPatch);
    
    if (_retain_vector) {
        record.patches = patch_data;
    }

    if (_load_as_texture) {
        record.patch_texture.reset(new GL::TextureBuffer(data_size, GL_RGB32F));
        record.patch_texture->load((void*)patch_data.data());
    }

    if (_load_as_opencl_buffer) {
        vector<vec4> cp_data;
        cp_data.reserve(record.patch_count * 16);

        for (auto patch : patch_data) {
            for (int i = 0; i < 16; ++i) {
                cp_data.push_back(vec4(patch.P[0][i],1));
            }
        }
        
        record.opencl_buffer.reset(new CL::Buffer(*_opencl_device, cp_data.size() * sizeof(vec4), CL_MEM_READ_ONLY, "patch-data"));
        CL::Event e = _opencl_queue->enq_write_buffer(*(record.opencl_buffer), (void*)cp_data.data(),
                                                      cp_data.size() * sizeof(vec4), "Patch transfer", CL::Event());
        _opencl_queue->wait_for_events(e);
    }
        
    _is_set_up = true;
}


void Reyes::PatchIndex::delete_patches(void* handle)
{
    assert(are_patches_loaded(handle));

    _index.erase(handle);
}


const vector<BezierPatch>& Reyes::PatchIndex::get_patch_vector(void* handle)
{
    assert(_retain_vector);
        
    return _index[handle].patches;
}


GL::TextureBuffer& Reyes::PatchIndex::get_patch_texture(void* handle)
{
    assert(_load_as_texture);

    return *(_index[handle].patch_texture);            
}


CL::Buffer* Reyes::PatchIndex::get_opencl_buffer(void* handle)
{
    assert(_load_as_opencl_buffer);

    return _index[handle].opencl_buffer.get();
}



size_t Reyes::PatchIndex::get_patch_count(void* handle)
{
    return _index[handle].patch_count;
}
