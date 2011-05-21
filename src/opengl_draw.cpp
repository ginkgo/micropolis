
#include "opengl_draw.h"
#include "Config.h"

struct OGLPatchDrawer : public PatchDrawer
{
    void draw_patch(const BezierPatch& patch);
};

struct OGLWirePatchDrawer : public PatchDrawer
{
    int patch_count;

    void draw_patch(const BezierPatch& patch);
};

#define P(pi,pj) patch.P[pi][pj]
void OGLWirePatchDrawer::draw_patch(const BezierPatch& patch)
{
    patch_count++;

    vec3 p;

    glColor3f(1,1,1);

    glBegin(GL_LINE_LOOP);
    
    int n = 4;

    for (int i = 0; i < n; ++i) {
        float t = float(i)/n;

        eval_spline(P(0,0), P(1,0), P(2,0), P(3,0), t, p);
        glVertex3f(p.x, p.y, p.z);
    }
    
    for (int i = 0; i < n; ++i) {
        float t = float(i)/n;

        eval_spline(P(3,0), P(3,1), P(3,2), P(3,3), t, p);
        glVertex3f(p.x, p.y, p.z);
    }
    
    for (int i = 0; i < n; ++i) {
        float t = float(i)/n;

        eval_spline(P(3,3), P(2,3), P(1,3), P(0,3), t, p);
        glVertex3f(p.x, p.y, p.z);
    }
    
    for (int i = 0; i < n; ++i) {
        float t = float(i)/n;

        eval_spline(P(0,3), P(0,2), P(0,1), P(0,0), t, p);
        glVertex3f(p.x, p.y, p.z);
    }

    glEnd();
}

void OGLPatchDrawer::draw_patch (const BezierPatch& patch)
{
    const int n = 20;

    vec3 row[n+1];
    vec3 nrow[n+1];

    for (int i = 0; i <= n; ++i) {
        float t = float(i)/n;
        eval_patch_n(patch, t, 0, row[i], nrow[i]);
    }
    
    for (int j = 1; j <= n; ++j) {
        float s = float(j)/n;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= n; ++i) {
            float t = float(i)/n;
            glColor3fv(glm::value_ptr(nrow[i]));
            glVertex3fv(glm::value_ptr(row[i]));
            eval_patch_n(patch, t, s, row[i], nrow[i]);
            glColor3fv(glm::value_ptr(nrow[i]));
            glVertex3fv(glm::value_ptr(row[i]));
        }
        glEnd();
    }
}

void ogl_main(vector<BezierPatch>& patches)
{
    ivec2 size = config.window_size();   
    bool running = true;

    glEnable(GL_DEPTH_TEST);

    float s = 8;

    double last = glfwGetTime();
    double time_diff = 0;

    //OGLPatchDrawer patch_drawer;
    OGLWirePatchDrawer wire_patch_drawer;

    double last_patch_count = last;

    while (running) {

        double now = glfwGetTime();
        time_diff = now - last;
        last = now;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        PerspectiveProjection projection(60, 0.01, size);

        mat4 proj;
        projection.calc_projection(proj);
                                         
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(proj));
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        mat4 view;
        view *= glm::translate<float>(0,-2,-s);
        view *= glm::rotate<float>(-70, 1,0,0);
        view *= glm::rotate<float>(now * 10, 0,0,1);
        //view *= glm::rotate<float>(90, 0,0,1);

        mat4x3 view4x3(view);
        BezierPatch transformed;

        for (size_t i = 0; i < patches.size(); ++i) {
            transform_patch(patches[i], view4x3, transformed);

            // if (glfwGetKey(GLFW_KEY_SPACE)) 
                split_n_draw(transformed, projection, wire_patch_drawer);
            // else
            //     split_n_draw(transformed, projection, patch_drawer);
        }

        glfwSwapBuffers();

        if (last-last_patch_count > 1) {
            cout << wire_patch_drawer.patch_count << " patches." << endl;
            last_patch_count = last;
        }

        wire_patch_drawer.patch_count = 0;

        float fps, mspf;
        calc_fps(fps, mspf);

        if (glfwGetKey( GLFW_KEY_UP )) {
            s += time_diff;
        }

        if (glfwGetKey( GLFW_KEY_DOWN )) {
            s -= time_diff;
        }

        // Check if the window has been closed
        running = running && !glfwGetKey( GLFW_KEY_ESC );
        running = running && !glfwGetKey( 'Q' );
        running = running && glfwGetWindowParam( GLFW_OPENED );    
    }
}
