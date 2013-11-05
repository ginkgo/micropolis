#include "BoundNSplit.h"


#include "PatchesIndex.h"
#include "Config.h"
#include "Statistics.h"


using namespace Reyes;

#define BATCH_SIZE config.reyes_patches_per_pass()

Reyes::BoundNSplit::BoundNSplit(shared_ptr<PatchesIndex>& patch_index)
    : _patch_index(patch_index)
    , _active_handle(nullptr)

    , _prefix_sum(BATCH_SIZE)
      
    , _stack_min(BATCH_SIZE * config.max_split_depth() * sizeof(vec2))
    , _stack_max(BATCH_SIZE * config.max_split_depth() * sizeof(vec2))
    , _stack_pid(BATCH_SIZE * config.max_split_depth() * sizeof(GLuint))
      
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
{
}


void Reyes::BoundNSplit::init(void* patches_handle, const mat4& matrix, const Projection* projection)
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

    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid);
    
    _init_ranges.unbind();

    _stack_height = patch_count;
    
    mat4 proj;
    projection->calc_projection_with_aspect_correction(proj);
    
    _mvp = proj * matrix;

    _framebuffer_matrix = mat3(glm::scale(vec3(config.window_size().x, config.window_size().y, 1))
                               * glm::translate(vec3(0.5f,0.5f,0.5f))
                               * glm::scale(vec3(0.5f,0.5f,0.5f)));
    _screen_min = vec3(0,0,-1);
    _screen_max = vec3(config.window_size().x, config.window_size().y, 1);

}


bool Reyes::BoundNSplit::done()
{
    return _stack_height == 0;
}


void Reyes::BoundNSplit::do_bound_n_split(GL::IndirectVBO& vbo)
{
    size_t batch_size = std::min(_stack_height, config.reyes_patches_per_pass());
    size_t batch_offset = _stack_height - batch_size;

    GL::Tex& patches_texture = _patch_index->get_patch_texture(_active_handle);

    // Apply Bound & Split
    _bound_n_split.bind();


    patches_texture.bind();
    GL::Buffer::bind_all(GL_SHADER_STORAGE_BUFFER, 0,
                         _stack_min, _stack_max, _stack_pid,
                         _flag_pad, _split_pad_pid, _split_pad1_min, _split_pad1_max, _split_pad2_min, _split_pad2_max);


    _bound_n_split.set_uniform("batch_size", (GLuint)batch_size);
    _bound_n_split.set_uniform("batch_offset", (GLuint)batch_offset);
    _bound_n_split.set_uniform("mvp", _mvp);
    _bound_n_split.set_uniform("framebuffer_matrix", _framebuffer_matrix);
    _bound_n_split.set_uniform("screen_min", _screen_min);
    _bound_n_split.set_uniform("screen_max", _screen_max);
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
                           _flag_pad, _split_pad_pid, _split_pad1_min, _split_pad1_max, _split_pad2_min, _split_pad2_max);    

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
    _create_geometry_for_ranges.set_buffer("vertex_buffer", vbo.get_vertex_buffer());

    _create_geometry_for_ranges.dispatch(1, (batch_size-1)/16 +1);
    
    GL::Buffer::unbind_all(_stack_min, _stack_max, _stack_pid,
                           _flag_pad, _summed_flags, vbo.get_vertex_buffer());

    _create_geometry_for_ranges.unbind();

    vbo.load_indirection(0, 1, 0, 0);

    _stack_height -= batch_size;

}


/*----------------------------------------------------------------------------*/
// Implementation of utility functions

void Reyes::vsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1)
{
    r0 = {r.range, r.depth + 1, r.patch_id};
    r1 = {r.range, r.depth + 1, r.patch_id};
        
    float cy = (r.range.min.y + r.range.max.y) * 0.5f;

    r0.range.min.y = cy;
    r1.range.max.y = cy;
}
    
void Reyes::hsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1)
{
    r0 = {r.range, r.depth + 1, r.patch_id};
    r1 = {r.range, r.depth + 1, r.patch_id};

    float cx = (r.range.min.x + r.range.max.x) * 0.5f;

    r0.range.min.x = cx;
    r1.range.max.x = cx;
}
    
void Reyes::bound_patch_range (const PatchRange& r, const BezierPatch& p, const mat4& mv, const mat4& mvp,
                               BBox& box, float& vlen, float& hlen)
{
    const size_t RES = 3;
        
    //vec2 pp[RES][RES];
    vec3 ps[RES][RES];
    vec3 pos;
     
    box.clear();
        
    for (int iu = 0; iu < RES; ++iu) {
        for (int iv = 0; iv < RES; ++iv) {
            float u = r.range.min.x + (r.range.max.x - r.range.min.x) * iu * (1.0f / (RES-1));
            float v = r.range.min.y + (r.range.max.y - r.range.min.y) * iv * (1.0f / (RES-1));

            eval_patch(p, u, v, pos);

            vec3 pt = vec3(mv * vec4(pos,1));
                
            box.add_point(pt);

            // pp[iu][iv] = project(mvp * vec4(pos,1));
            ps[iu][iv] = pt;
        }
    }

    vlen = 0;
    hlen = 0;

    for (int i = 0; i < RES; ++i) {
        float h = 0, v = 0;
        for (int j = 0; j < RES-1; ++j) {
            // v += glm::distance(pp[j][i], pp[j+1][i]);
            // h += glm::distance(pp[i][j], pp[i][j+1]);
            v += glm::distance(ps[j][i], ps[j+1][i]);
            h += glm::distance(ps[i][j], ps[i][j+1]);
        }
        vlen = maximum(v, vlen);
        hlen = maximum(h, hlen);
    }
}
