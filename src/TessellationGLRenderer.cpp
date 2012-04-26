#include "TessellationGLRenderer.h"

#include "Config.h"
#include "Projection.h"
#include "Statistics.h"

Reyes::TessellationGLRenderer::TessellationGLRenderer():
    _shader("tessellation"),
    _vbo(16 * config.reyes_patches_per_pass())
{
        

}


Reyes::TessellationGLRenderer::~TessellationGLRenderer()
{

}


void Reyes::TessellationGLRenderer::prepare()
{   
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPatchParameteri(GL_PATCH_VERTICES, 16);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_shader.bind();

    float patch_size = config.reyes_patch_size();
    _shader.set_uniform("tess_level", patch_size);

    statistics.start_render();
}

void Reyes::TessellationGLRenderer::finish()
{
    flush();

    _shader.unbind();

    get_errors();

    glfwSwapBuffers();
    
    statistics.end_render();
}

void Reyes::TessellationGLRenderer::set_projection(const Projection& projection)
{
    mat4 proj;

    projection.calc_projection(proj);

    _shader.set_uniform("proj", proj);
}

void Reyes::TessellationGLRenderer::draw_patch(const BezierPatch& patch)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            _vbo.vertex(patch.P[i][j]);
        }
    }

    if (_vbo.full())
    {
        flush();
    }
}

void Reyes::TessellationGLRenderer::flush()
{
    if (_vbo.empty()) return;

    _vbo.send_data();
    _vbo.draw(GL_PATCHES, _shader);
    _vbo.clear();
}