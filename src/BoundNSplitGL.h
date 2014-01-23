#pragma once

#include "common.h"

#include "GL/ComputeShader.h"
#include "GL/PrefixSum.h"
#include "GL/Texture.h"
#include "GL/VBO.h"
#include "Patch.h"
#include "PatchRange.h"
#include "Projection.h"

namespace Reyes
{

    class PatchIndex;    


    class BoundNSplitGL
    {
        
    public:

        virtual ~BoundNSplitGL() {};

        virtual void init(void* patches_handle, const mat4& matrix, const Projection* projection) = 0;
        virtual bool done() = 0;

        virtual void do_bound_n_split(GL::IndirectVBO& vbo) = 0;
        
    };

    
    class BoundNSplitGLLocal : public BoundNSplitGL
    {

        shared_ptr<PatchIndex> _patch_index;

        GL::ComputeShader _bound_n_split_kernel;
        GL::ComputeShader _clear_out_range_cnt_kernel;
        GL::ComputeShader _init_count_buffers_kernel;
        GL::ComputeShader _init_range_buffers_kernel;
        GL::ComputeShader _setup_indirection_kernel;

        void* _active_handle;
        mat4 _active_matrix;

        size_t _in_buffer_size;
        size_t _in_buffer_stride;

        GL::Buffer _in_pids_buffer;
        GL::Buffer _in_mins_buffer;
        GL::Buffer _in_maxs_buffer;
        GL::Buffer _in_range_cnt_buffer;

        GL::Buffer _out_range_cnt_buffer;

        bool _done;
        int _iteration_count;
        
    public:

        BoundNSplitGLLocal(shared_ptr<PatchIndex>& patch_index);
        ~BoundNSplitGLLocal();

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        void do_bound_n_split(GL::IndirectVBO& vbo);
        
    };

    
    class BoundNSplitGLMultipass : public BoundNSplitGL
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
        
        GL::ComputeShader _bound_n_split;
        GL::ComputeShader _copy_ranges;
        GL::ComputeShader _create_geometry_for_ranges;
        GL::ComputeShader _init_ranges;
        GL::ComputeShader _setup_indirection;

        GLuint _bound_n_split_timer;
        GLuint _dice_n_raster_timer;
        
    public:

        BoundNSplitGLMultipass(shared_ptr<PatchIndex>& patch_index);
        ~BoundNSplitGLMultipass();

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        void do_bound_n_split(GL::IndirectVBO& vbo);
        
    };


}
