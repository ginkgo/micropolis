
#include "PatchesIndex.h"

#include "common.h"


Reyes::PatchesIndex::PatchesIndex()
    : _is_set_up(false)
    , _load_as_texture(false)
    , _load_as_opencl_buffer(false)
    , _retain_vector(false)
{
        
}


Reyes::PatchesIndex::~PatchesIndex()
{

}


void Reyes::PatchesIndex::enable_load_texture()
{
    assert(!_is_set_up);
    _load_as_texture = true;
}


void Reyes::PatchesIndex::enable_load_opencl_buffer()
{
    assert(!_is_set_up);
    _load_as_opencl_buffer = true;
}


void Reyes::PatchesIndex::enable_retain_vector()
{
    assert(!_is_set_up);
    _retain_vector = true;
}


bool Reyes::PatchesIndex::are_patches_loaded(void* handle)
{
    return _index.count(handle) > 0;
}


void Reyes::PatchesIndex::load_patches(void* handle, const vector<BezierPatch>& patch_data)
{
    assert(handle != nullptr);
    
    PatchData& record = _index[handle];

    record.patch_count = patch_data.size();
    
    if (_retain_vector) {
        record.patches = patch_data;
    }

    if (_load_as_texture) {
        record.patch_texture.reset(new GL::TextureBuffer(patch_data.size() * sizeof(BezierPatch), GL_RGB32F));
        record.patch_texture->load((void*)patch_data.data());
    }

    if (_load_as_opencl_buffer) {
        #warning TODO
    }
        
    _is_set_up = true;
}


void Reyes::PatchesIndex::delete_patches(void* handle)
{
    assert(are_patches_loaded(handle));

    _index.erase(handle);
}


const vector<BezierPatch>& Reyes::PatchesIndex::get_patch_vector(void* handle)
{
    assert(_retain_vector);
        
    return _index[handle].patches;
}


GL::TextureBuffer& Reyes::PatchesIndex::get_patch_texture(void* handle)
{
    assert(_load_as_texture);

    return *(_index[handle].patch_texture);            
}


size_t Reyes::PatchesIndex::get_patch_count(void* handle)
{
    return _index[handle].patch_count;
}
