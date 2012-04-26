#ifndef TESSELLATIONGLRENDERER_H
#define TESSELLATIONGLRENDERER_H

#include "Patch.h"
#include "PatchDrawer.h"

#include "Shader.h"
#include "VBO.h"

namespace Reyes
{

    class TessellationGLRenderer : public PatchDrawer
    {
        GL::Shader _shader;
        GL::VBO _vbo;

    public:

        TessellationGLRenderer();
        ~TessellationGLRenderer();

        virtual void prepare();
        virtual void finish();

        virtual void set_projection(const Projection& projection);
        virtual void draw_patch(const BezierPatch& patch);

        void flush();
    };

}

#endif