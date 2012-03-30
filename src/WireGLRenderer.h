#ifndef WIREGLRENDERER_H
#define WIREGLRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

#include "Shader.h"
#include "VBO.h"

namespace Reyes
{

    class WireGLRenderer : public PatchDrawer
    {
		GL::Shader _shader;
		GL::VBO _vbo;
		
        public:

        WireGLRenderer();
        ~WireGLRenderer() {};

        virtual void prepare();
        virtual void finish();
        
        virtual void set_projection(const Projection& projection);
        virtual void draw_patch (const BezierPatch& patch);
    };
    
}

#endif
