#include "BoundNSplitGL.h"

#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"

#include "utility.h"

#define BATCH_SIZE config.reyes_patches_per_pass()
#define WORK_GROUP_CNT 32
#define WORK_GROUP_SIZE 64
#define MAX_SPLIT_DEPTH 8
#define MAX_BNS_ITERATIONS 200

Reyes::BoundNSplitGLLocal::BoundNSplitGLLocal(shared_ptr<PatchIndex>& patch_index)
    : _patch_index(patch_index)

    , _bound_n_split_kernel("local_bound_n_split")
    , _clear_out_range_cnt_kernel("local_clear_out_range_cnt")
    , _init_count_buffers_kernel("local_init_count_buffers")
    , _init_range_buffers_kernel("local_init_range_buffers")
    , _setup_indirection_kernel("local_setup_indirection")

    , _in_buffer_size(0)
    , _in_buffer_stride(0)
    , _in_pids_buffer(0)
    , _in_depths_buffer(0)
    , _in_mins_buffer(0)
    , _in_maxs_buffer(0)
    , _in_range_cnt_buffer(WORK_GROUP_CNT * sizeof(GLint))

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

        size_t item_count = round_up_by<size_t>(_in_buffer_size, WORK_GROUP_CNT) + MAX_SPLIT_DEPTH * WORK_GROUP_SIZE * WORK_GROUP_CNT;

        assert(item_count % WORK_GROUP_CNT == 0);        
        _in_buffer_stride = item_count / WORK_GROUP_CNT;
        
        _in_pids_buffer.resize(item_count * sizeof(GLint));
        _in_depths_buffer.resize(item_count * sizeof(GLint));
        _in_mins_buffer.resize(item_count * sizeof(vec2));
        _in_maxs_buffer.resize(item_count * sizeof(vec2));
    }

    // Init count buffers
    _init_count_buffers_kernel.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0, _in_range_cnt_buffer);
    
    _init_count_buffers_kernel.set_uniform("patch_count", (GLuint)patch_count);
    _init_count_buffers_kernel.set_buffer("in_range_cnt", _in_range_cnt_buffer);
    
    _init_count_buffers_kernel.dispatch(1);
    
    GL::Buffer::unbind_all(_in_range_cnt_buffer);
    _init_count_buffers_kernel.unbind();

    // Init range buffers
    _init_range_buffers_kernel.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer);
    
    _init_range_buffers_kernel.set_uniform("patch_count", (GLuint)patch_count);
    _init_range_buffers_kernel.set_uniform("buffer_stride", (GLuint)_in_buffer_stride);
    _init_range_buffers_kernel.set_buffer("in_pids", _in_pids_buffer);
    _init_range_buffers_kernel.set_buffer("in_mins", _in_mins_buffer);
    _init_range_buffers_kernel.set_buffer("in_maxs", _in_maxs_buffer);
    
    _init_range_buffers_kernel.dispatch(round_up_div<int>(patch_count, WORK_GROUP_SIZE));
    GL::Buffer::unbind_all(_in_pids_buffer, _in_mins_buffer, _in_maxs_buffer);
    _init_range_buffers_kernel.unbind();

    _done = false;
}


bool Reyes::BoundNSplitGLLocal::done()
{
    return _done;
}


void Reyes::BoundNSplitGLLocal::do_bound_n_split(GL::IndirectVBO& vbo)
{
    // Init out_range_cnt buffer
    _clear_out_range_cnt_kernel.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0, _out_range_cnt_buffer);
    _clear_out_range_cnt_kernel.set_buffer("out_range_cnt", _out_range_cnt_buffer);

    _clear_out_range_cnt_kernel.dispatch(1);
    GL::Buffer::unbind_all(_out_range_cnt_buffer);
    _clear_out_range_cnt_kernel.unbind();
    
    // Start bound&split kernel
    _bound_n_split_kernel.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                         vbo.get_vertex_buffer(), _out_range_cnt_buffer);
    _bound_n_split_kernel.set_buffer("in_pids", _in_pids_buffer);
    _bound_n_split_kernel.set_buffer("in_mins", _in_mins_buffer);
    _bound_n_split_kernel.set_buffer("in_maxs", _in_maxs_buffer);
    _bound_n_split_kernel.set_buffer("in_range_cnt", _in_range_cnt_buffer);
    _bound_n_split_kernel.set_buffer("vertex_buffer", vbo.get_vertex_buffer());
    _bound_n_split_kernel.set_buffer("out_range_cnt", _out_range_cnt_buffer);

    _bound_n_split_kernel.set_uniform("in_buffer_stride", (GLuint)_in_buffer_stride);
    _bound_n_split_kernel.set_uniform("out_buffer_size", (GLuint)BATCH_SIZE);
    
    _bound_n_split_kernel.dispatch(WORK_GROUP_CNT);
    GL::Buffer::unbind_all(_in_pids_buffer, _in_mins_buffer, _in_maxs_buffer, _in_range_cnt_buffer,
                           vbo.get_vertex_buffer(), _out_range_cnt_buffer);
    _bound_n_split_kernel.unbind();


    // Setup indirection buffer from result of bound&split kernel
    _setup_indirection_kernel.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _out_range_cnt_buffer, vbo.get_indirection_buffer());
    _setup_indirection_kernel.set_uniform("batch_size", vbo.get_max_vertex_count()/4);
    _setup_indirection_kernel.set_buffer("out_range_cnt", _out_range_cnt_buffer);
    _setup_indirection_kernel.set_buffer("indirection_buffer", vbo.get_indirection_buffer());

    _setup_indirection_kernel.dispatch(1);
    
    GL::Buffer::unbind_all(_out_range_cnt_buffer, vbo.get_indirection_buffer());
    _setup_indirection_kernel.unbind();

    _done = true;

    
}
    
