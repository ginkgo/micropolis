#pragma once

#include "common.h"

#include "BoundNSplitGL.h"


namespace Reyes
{

    class PatchIndex;


    
    class BoundNSplitGLCPU : public BoundNSplitGL
    {

        shared_ptr<PatchIndex> _patch_index;

        void* _active_handle;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;
        
    public:

        BoundNSplitGLCPU(shared_ptr<PatchIndex>& patch_index);

        void init(void* patches_handle, const mat4& matrix, const Projection* projection);
        bool done();

        void do_bound_n_split(GL::IndirectVBO& vbo);


    private:

        
        static void vsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void hsplit_range(const PatchRange& r, vector<PatchRange>& stack);
        static void bound_patch_range (const PatchRange& r, const BezierPatch& p,
                                       const mat4& mv, const mat4& mvp,
                                       BBox& box, float& vlen, float& hlen);
        
    };

}
