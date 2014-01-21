#include "BoundNSplitGL.h"


#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"


using namespace Reyes;

#define BATCH_SIZE config.reyes_patches_per_pass()
#define MAX_SPLIT config.max_split_depth()

Reyes::BoundNSplitGLMultipass::BoundNSplitGLMultipass(shared_ptr<PatchIndex>& patch_index)
    : _patch_index(patch_index)
    , _active_handle(nullptr)

    , _prefix_sum(BATCH_SIZE)
      
    , _stack_min(BATCH_SIZE * (MAX_SPLIT+1) * sizeof(vec2))
    , _stack_max(BATCH_SIZE * (MAX_SPLIT+1) * sizeof(vec2))
    , _stack_pid(BATCH_SIZE * (MAX_SPLIT+1) * sizeof(GLuint))
      
    , _flag_pad(BATCH_SIZE * sizeof(uvec2))
    , _split_pad_pid(BATCH_SIZE * sizeof(uint))
    , _split_pad1_min(BATCH_SIZE * sizeof(vec2))
    , _split_pad1_max(BATCH_SIZE * sizeof(vec2))
    , _split_pad2_min(BATCH_SIZE * sizeof(vec2))
    , _split_pad2_max(BATCH_SIZE * sizeof(vec2))
    
    , _summed_flags(BATCH_SIZE * sizeof(uvec2))
    , _flag_total(sizeof(uvec2))
    
    , _stack_height(0)
    , _init_ranges("init_ranges")
    , _bound_n_split("bound_n_split")
    , _create_geometry_for_ranges("create_geometry_for_ranges")
    , _copy_ranges("copy_ranges")
    , _setup_indirection("setup_indirection")

    , _bound_n_split_timer(0)
    , _dice_n_raster_timer(0)
{
    glGenQueries(1, &_bound_n_split_timer);
    glGenQueries(1, &_dice_n_raster_timer);
}


Reyes::BoundNSplitGLMultipass::~BoundNSplitGLMultipass()
{
    glDeleteQueries(1, &_bound_n_split_timer);
    glDeleteQueries(1, &_dice_n_raster_timer);
}


void Reyes::BoundNSplitGLMultipass::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    
    _active_handle = patches_handle;

    size_t patch_count = _patch_index->get_patch_count(patches_handle);

    _init_ranges.bind();

    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _stack_min, _stack_max, _stack_pid);

    _init_ranges.set_uniform("patch_count", (GLint)patch_count);
    _init_ranges.set_buffer("stack_min", _stack_min);
    _init_ranges.set_buffer("stack_max", _stack_max);
    _init_ranges.set_buffer("stack_pid", _stack_pid);
    
    _init_ranges.dispatch((patch_count-1)/64 + 1);
    glFlush();
    
    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid);
    
    _init_ranges.unbind();

    _stack_height = patch_count;
    
    mat4 proj;
    projection->calc_projection(proj);

    vec2 hwin(config.window_size().x/2.0f, config.window_size().y/2.0f);
    
    _mv = matrix;

    _proj = proj;

    _screen_matrix = mat3(glm::scale(vec3(hwin.x, hwin.y, 1)));
    
    _screen_min = vec3(-hwin.x,-hwin.y,-1);
    _screen_max = vec3( hwin.x, hwin.y, 1);

    _bound_n_split.bind();
    _bound_n_split.set_uniform("near", projection->near());
    _bound_n_split.set_uniform("far", projection->far());
    _bound_n_split.set_uniform("proj_f", projection->f());
    _bound_n_split.set_uniform("screen_size", projection->viewport());
    _bound_n_split.set_uniform("cull_ribbon", (float)config.cull_ribbon());
    _bound_n_split.unbind();

}


bool Reyes::BoundNSplitGLMultipass::done()
{
    
    glEndQuery(GL_TIME_ELAPSED); // dice_n_raster_timer
    
    ivec2 total;

    _flag_total.bind(GL_ARRAY_BUFFER);
    _flag_total.read_data(&total, sizeof(ivec2));
    _flag_total.unbind();

    _stack_height += total.x * 2;

    if (false) {
        cout << format("%1% split, %2% drawn, stack_height: %3%") % total.x  % total.y % _stack_height << endl;
    }

    statistics.add_patches((int)total.y);

    if (_stack_height > BATCH_SIZE * MAX_SPLIT) {
        cerr << "Stack overflow pending - ABORT" << endl;
        return true;
    }

    GLint result_available;
    
    glGetQueryObjectiv(_bound_n_split_timer, GL_QUERY_RESULT_AVAILABLE, &result_available);

    if (result_available) {
        GLuint64 ns_elapsed;
        glGetQueryObjectui64v(_bound_n_split_timer, GL_QUERY_RESULT, &ns_elapsed);

        statistics.add_bound_n_split_time(ns_elapsed);
    } else {
        cerr << "Timer query not ready" << endl;
    }
    
    glGetQueryObjectiv(_dice_n_raster_timer, GL_QUERY_RESULT_AVAILABLE, &result_available);

    if (result_available) {
        GLuint64 ns_elapsed;
        glGetQueryObjectui64v(_dice_n_raster_timer, GL_QUERY_RESULT, &ns_elapsed);

        statistics.add_dice_n_raster_time(ns_elapsed);
    } else {
        cerr << "Timer query not ready" << endl;
    }
 
    statistics.inc_pass_count(1);   

    assert(_stack_height < BATCH_SIZE * MAX_SPLIT);
    
    return _stack_height == 0;
}


void Reyes::BoundNSplitGLMultipass::do_bound_n_split(GL::IndirectVBO& vbo)
{
    glBeginQuery(GL_TIME_ELAPSED, _bound_n_split_timer);
    
    size_t batch_size = std::min(_stack_height, config.reyes_patches_per_pass());
    size_t batch_offset = _stack_height - batch_size;

    GL::Tex& patches_texture = _patch_index->get_patch_texture(_active_handle);

    // Apply Bound & Split
    _bound_n_split.bind();

    patches_texture.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _stack_min, _stack_max, _stack_pid,
                         _flag_pad, _split_pad_pid,
                         _split_pad1_min, _split_pad1_max,
                         _split_pad2_min, _split_pad2_max);


    _bound_n_split.set_uniform("batch_size", (GLuint)batch_size);
    _bound_n_split.set_uniform("batch_offset", (GLuint)batch_offset);
    _bound_n_split.set_uniform("mv", _mv);
    _bound_n_split.set_uniform("proj", _proj);
    _bound_n_split.set_uniform("screen_matrix", _screen_matrix);
    _bound_n_split.set_uniform("screen_min", _screen_min);
    _bound_n_split.set_uniform("screen_max", _screen_max);
    _bound_n_split.set_uniform("max_split_depth", (GLuint)config.max_split_depth());
    _bound_n_split.set_uniform("split_limit", config.bound_n_split_limit());
    _bound_n_split.set_uniform("patches", patches_texture);
    
    _bound_n_split.set_buffer("stack_min", _stack_min);
    _bound_n_split.set_buffer("stack_max", _stack_max);
    _bound_n_split.set_buffer("stack_pid", _stack_pid);
    _bound_n_split.set_buffer("flag_pad", _flag_pad);
    _bound_n_split.set_buffer("split_pad_pid", _split_pad_pid);
    _bound_n_split.set_buffer("split_pad1_min", _split_pad1_min);
    _bound_n_split.set_buffer("split_pad1_max", _split_pad1_max);
    _bound_n_split.set_buffer("split_pad2_min", _split_pad2_min);
    _bound_n_split.set_buffer("split_pad2_max", _split_pad2_max);
    
    _bound_n_split.dispatch((batch_size-1)/64 + 1);
    
    patches_texture.unbind();
    
    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid,
                           _flag_pad, _split_pad_pid,
                           _split_pad1_min, _split_pad1_max,
                           _split_pad2_min, _split_pad2_max);    

    // Apply Prefix Sum
    _prefix_sum.apply(batch_size, _flag_pad, _summed_flags, _flag_total);
    
    // Create geometry
    _create_geometry_for_ranges.bind();

    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _stack_min, _stack_max, _stack_pid,
                         _flag_pad, _summed_flags, vbo.get_vertex_buffer());

    _create_geometry_for_ranges.set_uniform("batch_size", (int)batch_size);
    _create_geometry_for_ranges.set_uniform("batch_offset", (int)batch_offset);
    _create_geometry_for_ranges.set_buffer("stack_min", _stack_min);
    _create_geometry_for_ranges.set_buffer("stack_max", _stack_max);
    _create_geometry_for_ranges.set_buffer("stack_pid", _stack_pid);
    _create_geometry_for_ranges.set_buffer("flag_pad", _flag_pad);
    _create_geometry_for_ranges.set_buffer("summed_flags", _summed_flags);
    _create_geometry_for_ranges.set_buffer("vertex_buffer", vbo.get_vertex_buffer());

    _create_geometry_for_ranges.dispatch(1, (batch_size-1)/16 +1);
    
    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid,
                           _flag_pad, _summed_flags, vbo.get_vertex_buffer());

    _create_geometry_for_ranges.unbind();

    // Place split patches back on the stack
    _copy_ranges.bind();

    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _stack_min, _stack_max, _stack_pid,
                         _flag_pad, _summed_flags,
                         _split_pad_pid, _split_pad1_min, _split_pad1_max, _split_pad2_min, _split_pad2_max);

    _copy_ranges.set_uniform("batch_size", (int)batch_size);
    _copy_ranges.set_uniform("batch_offset", (int)batch_offset);
    _copy_ranges.set_buffer("stack_min", _stack_min);
    _copy_ranges.set_buffer("stack_max", _stack_max);
    _copy_ranges.set_buffer("stack_pid", _stack_pid);
    _copy_ranges.set_buffer("flag_pad", _flag_pad);
    _copy_ranges.set_buffer("summed_flags", _summed_flags);
    _copy_ranges.set_buffer("split_pad_pid", _split_pad_pid);
    _copy_ranges.set_buffer("split_pad1_min", _split_pad1_min);
    _copy_ranges.set_buffer("split_pad1_max", _split_pad1_max);
    _copy_ranges.set_buffer("split_pad2_min", _split_pad2_min);
    _copy_ranges.set_buffer("split_pad2_max", _split_pad2_max);

    _copy_ranges.dispatch((batch_size-1)/64 + 1);
    
    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid,
                           _flag_pad, _summed_flags,
                           _split_pad_pid, _split_pad1_min, _split_pad1_max, _split_pad2_min, _split_pad2_max);
    
    _copy_ranges.unbind();

    // Set-up VBO indirection buffer
    _setup_indirection.bind();

    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _flag_total, vbo.get_indirection_buffer());

    _setup_indirection.set_buffer("flag_total", _flag_total);
    _setup_indirection.set_buffer("indirection_buffer", vbo.get_indirection_buffer());

    _setup_indirection.dispatch(1);
    
    GL::Buffer::unbind_all(_flag_total, vbo.get_indirection_buffer());
    
    _setup_indirection.unbind();
    
    _stack_height -= batch_size;

    glEndQuery(GL_TIME_ELAPSED); // bound_n_split_timer
    glBeginQuery(GL_TIME_ELAPSED, _dice_n_raster_timer);
}

