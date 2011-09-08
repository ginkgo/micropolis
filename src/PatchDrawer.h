#ifndef PATCHDRAWER_H
#define PATCHDRAWER_H

#include "Patch.h"

namespace Reyes
{

    class Projection;

    struct PatchDrawer
    {
        virtual ~PatchDrawer() {};

        virtual void prepare() = 0;
        virtual void finish() = 0;
        
        virtual void set_projection(const Reyes::Projection& projection) = 0;
        virtual void draw_patch(const BezierPatch& patch) = 0;
    };

    void bound_n_split(const BezierPatch& patch, 
                       const Reyes::Projection& projection,
                       PatchDrawer& patch_drawer);
}

#endif
