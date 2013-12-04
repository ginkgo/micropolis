#pragma once

#include "common.h"

#include "GL/ComputeShader.h"
#include "GL/PrefixSum.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "Projection.h"

namespace Reyes
{
    struct PatchRange
    {
        Bound range;
        size_t depth;
        size_t patch_id;
    };

    class PatchIndex;    

    class BoundNSplit
    {

        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;

        mat4 _mv;
        mat4 _proj;

        mat3 _screen_matrix;
        vec3 _screen_min;
        vec3 _screen_max;

        GL::PrefixSum _prefix_sum;

        GL::Buffer _stack_min;
        GL::Buffer _stack_max;
        GL::Buffer _stack_pid;

        GL::Buffer _flag_pad;
        GL::Buffer _split_pad_pid;
        GL::Buffer _split_pad1_min;
        GL::Buffer _split_pad1_max;
        GL::Buffer _split_pad2_min;
        GL::Buffer _split_pad2_max;

        GL::Buffer _summed_flags;
        GL::Buffer _flag_total;

        size_t _stack_height;
        
        GL::ComputeShader _init_ranges;
        GL::ComputeShader _bound_n_split;
        GL::ComputeShader _create_geometry_for_ranges;
        GL::ComputeShader _copy_ranges;
        GL::ComputeShader _setup_indirection;

        GLuint _bound_n_split_timer;
        GLuint _dice_n_raster_timer;
        
    public:

        BoundNSplit(shared_ptr<PatchIndex>& patch_index);
        ~BoundNSplit();

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        void do_bound_n_split(GL::IndirectVBO& vbo);
        
    };

    
    void vsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1);
    void hsplit_range(const PatchRange& r, PatchRange& r0, PatchRange& r1);
    void bound_patch_range (const PatchRange& r, const BezierPatch& p,
                            const mat4& mv, const mat4& mvp,
                            BBox& box, float& vlen, float& hlen);

}