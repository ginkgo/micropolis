#pragma once

#include "common.h"
#include "Projection.h"
#include "VBO.h"
#include "Patch.h"

namespace Reyes
{
    struct PatchRange
    {
        Bound range;
        size_t depth;
        size_t patch_id;
    };

    class PatchesIndex;    

    class BoundNSplit
    {

        shared_ptr<PatchesIndex> _patch_index;

        void* _active_handle;
        vector<PatchRange> _stack;

        mat4 _mv;
        mat4 _mvp;
        const Projection* _projection;
        
    public:

        BoundNSplit(shared_ptr<PatchesIndex>& patch_index);

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
