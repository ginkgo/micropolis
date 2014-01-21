#include "BoundNSplitGL.h"

#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"

#define BATCH_SIZE config.reyes_patches_per_pass()
#define WORK_GROUP_CNT config.local_bns_work_groups()
#define WORK_GROUP_SIZE 64
#define MAX_SPLIT_DEPTH config.max_split_depth()
#define MAX_BNS_ITERATIONS 200

Reyes::BoundNSplitGLLocal::BoundNSplitGLLocal(shared_ptr<PatchIndex>& patch_index)
    : _patch_index(patch_index)

    , _bound_n_split_kernel("local_bound_n_split")
    , _init_range_buffers_kernel("init_range_buffers")
    , _init_count_buffers_kernel("init_count_buffers")

    , _in_buffer_size(0)
    , _in_buffer_stride(0)
    , _in_pids_buffer(0)
    , _in_mins_buffer(0)
    , _in_maxs_buffer(0)
    , _in_range_cnt_buffer(WORK_GROUP_CNT * sizeof(GLint))

    , _out_pids_buffer(BATCH_SIZE * sizeof(int))
    , _out_mins_buffer(BATCH_SIZE * sizeof(vec2))
    , _out_maxs_buffer(BATCH_SIZE * sizeof(vec2))
    , _out_range_cnt_buffer(sizeof(GLint))
{
    patch_index->enable_load_texture();
}


Reyes::BoundNSplitGLLocal::~BoundNSplitGLLocal()
{

}


void Reyes::BoundNSplitGLLocal::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    GL::TextureBuffer& active_patch_buffer = _patch_index->get_patch_texture(patches_handle);
    _active_matrix = matrix;

    size_t patch_count = _patch_index->get_patch_count(patches_handle);

    if (_in_buffer_size < patch_count) {
        _in_buffer_size = patch_count;

        size_t item_count = round_up_by(_in_buffer_size, WORK_GROUP_CNT) + MAX_SPLIT_DEPTH * WORK_GROUP_SIZE * WORK_GROUP_CNT;

        assert(item_count % WORK_GROUP_CNT == 0);        
        _in_buffer_stride = item_count / WORK_GROUP_CNT;
        
        _in_pids_buffer.resize(item_count * sizeof(int));
        _in_mins_buffer.resize(item_count * sizeof(vec2));
        _in_maxs_buffer.resize(item_count * sizeof(vec2));
    }

    
}


bool Reyes::BoundNSplitGLLocal::done()
{
    return true;
}


void Reyes::BoundNSplitGLLocal::do_bound_n_split(GL::IndirectVBO& vbo)
{

}
    
