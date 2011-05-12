
#include "opengl_draw.h"
#include "Config.h"

#define P(pi,pj) patch.P[pi][pj]
void draw_patch_wire(const BezierPatch& patch)
{
    vec3 p;

    glColor3f(1,1,1);

    glBegin(GL_LINE_LOOP);
    
    int n = 5;

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

void draw_patch (const BezierPatch& patch)
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

    float last = glfwGetTime();
    float time_diff = 0;
    while (running) {

        float now = glfwGetTime();
        time_diff = now - last;
        last = now;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 vp = glm::inverse(glm::ortho<float>(0,size.x,0,size.y,-1,1));

        mat4 proj = glm::perspective<float>(60, float(size.x)/size.y, 0.001, 100);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(proj));
        glMatrixMode(GL_MODELVIEW);

        mat4 view;
        view *= glm::translate<float>(0,0,-s);
        view *= glm::rotate<float>(now * 29, 1,0,0);
        view *= glm::rotate<float>(now * 17, 0,1,0);
        view *= glm::rotate<float>(now * 13, 0,0,1);

        glLoadMatrixf(glm::value_ptr(view));

        mat4 mat = vp*proj*view;

        for (size_t i = 0; i < patches.size(); ++i) {
            project_patch(patches[i], mat);

            if (glfwGetKey(GLFW_KEY_SPACE)) 
                split_n_draw(7, patches[i], mat, draw_patch_wire);
            else
                split_n_draw(7, patches[i], mat, draw_patch);
        }

        glPopMatrix();

        glfwSwapBuffers();

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
