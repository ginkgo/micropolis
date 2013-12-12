#pragma once

#include "common.h"



namespace Reyes
{
    struct PatchRange
    {
        Bound range;
        size_t depth;
        size_t patch_id;

        PatchRange() {}
        
        // PatchRange(PatchRange& other)
        //     : range(other.range)
        //     , depth(other.depth)
        //     , patch_id(other.patch_id) {}
        
        // PatchRange(PatchRange&& other)
        //     : range(other.range)
        //     , depth(other.depth)
        //     , patch_id(other.patch_id) {}
        
        PatchRange(const Bound& range, size_t depth, size_t patch_id)
            : range(range)
            , depth(depth)
            , patch_id(patch_id) {}
        
        PatchRange(float xmin, float ymin, float xmax, float ymax,
                   size_t depth, size_t patch_id)
            : range(xmin,ymin,xmax,ymax)
            , depth(depth)
            , patch_id(patch_id) {}
    };
}
