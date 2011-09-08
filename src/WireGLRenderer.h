#ifndef WIREGLRENDERER_H
#define WIREGLRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

namespace Reyes
{

    class WireGLRenderer : public PatchDrawer
    {

        public:

        WireGLRenderer() {};
        ~WireGLRenderer() {};

        virtual void prepare();
        virtual void finish();
        
        virtual void set_projection(const Projection& projection);
        virtual void draw_patch (const BezierPatch& patch);
    };
    
}

#endif
