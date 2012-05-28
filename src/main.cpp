/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#include "common.h"

#include "Patch.h"
#include "Config.h"

// #include "opengl_draw.h"

#include "OpenCL.h"
#include "Reyes.h"

#include "Statistics.h"

#include "TessellationGLRenderer.h"

static bool close_window = false;
static int GLFWCALL window_close_callback( void )
{
    close_window = true;
    return GL_FALSE;
}

void mainloop()
{
    CL::Device device(config.platform_id(), config.device_id());

    if (config.verbose()) {
        device.print_info();
    }

    Reyes::Scene scene(new Reyes::PerspectiveProjection(75.0f, 0.01f, config.window_size()));
    scene.add_patches(config.input_file());

    Reyes::OGLSharedFramebuffer framebuffer();
    mat4 view;
    view *= glm::translate<float>(0,0,-4.5);
    view *= glm::rotate<float>(-90, 1,0,0);
    
    Reyes::WireGLRenderer wire_renderer;
    Reyes::PatchDrawer* renderer;
    
    switch (config.renderer_type()) {
    case Config::OPENCL:
        renderer = new Reyes::Renderer();
        break;
    case Config::GLTESS:
        renderer = new Reyes::TessellationGLRenderer();
        break;
    default:
        assert(0);
    }

    if(config.verbose() || !device.share_gl()) {
        cout << endl;
        cout << "Device is" << (device.share_gl() ? " " : " NOT ") << "shared." << endl << endl;
    }

    bool running = true;

    statistics.reset_timer();
    double last = glfwGetTime();
    while (running) {

        double now = glfwGetTime();
        double time_diff = now - last;
        last = now;

        if (glfwGetKey(GLFW_KEY_UP)) {
            view = glm::translate<float>(0,0,-time_diff) * view;
        }

        if (glfwGetKey(GLFW_KEY_DOWN)) {
            view = glm::translate<float>(0,0, time_diff) * view;
        }

        view *= glm::rotate<float>((float)time_diff * 5, 0,0,1);
        view *= glm::rotate<float>((float)time_diff * 7, 0,1,0);
        view *= glm::rotate<float>((float)time_diff * 11, 1,0,0);


        scene.set_view(view);

        if (glfwGetKey(GLFW_KEY_F3)) {
            scene.draw(wire_renderer);
            statistics.reset_timer();
        } else {
            scene.draw(*renderer);
        }

        statistics.update();


        // Check if the window has been closed
        running = running && !glfwGetKey( GLFW_KEY_ESC );
        running = running && !glfwGetKey( 'Q' );
        running = running && !close_window;
    }

    delete renderer;
}

void handle_arguments(int argc, char** argv)
{
    bool needs_resave;

    if (!Config::load_file("options.txt", config, needs_resave)) {
        cout << "Failed to load options.txt" << endl;
    }

    if (needs_resave) {
        if (config.verbose()) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!Config::save_file("options.txt", config)) {
            cout << "Failed to save options.txt" << endl;
        }
    }

    argc = config.parse_args(argc, argv);
    
}

int init_opengl(ivec2 window_size);

int main(int argc, char** argv)
{
    handle_arguments(argc, argv);

    ivec2 size = config.window_size();

    if (!init_opengl(size)) {
        return 1;
    }
    
    cout << endl;
    cout << "MICROPOLIS - A micropolygon rasterizer" << " (c) Thomas Weber 2012" << endl;
    cout << endl;

    glfwSetWindowTitle(config.window_title().c_str());
    glfwSetWindowCloseCallback(window_close_callback);

    try {

        mainloop();

    } catch (CL::Exception& e) {

        cerr << e.file() << ":" << e.line_no() << ": error: " <<  e.msg() << endl;

    }

    assert(statistics.opencl_memory == 0L);
    
    glfwCloseWindow();

    glfwTerminate();
    return 0;
}

/* 
 * Helper function that properly initializes the GLFW window before opening.
 */
int init_opengl(ivec2 size)
{
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return GL_FALSE;
    }

    int version = FLEXT_MAJOR_VERSION * 10 + FLEXT_MINOR_VERSION;

    // We can use this to setup the desired OpenGL version in GLFW
    //glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
    //glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, FLEXT_MAJOR_VERSION);
    //glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, FLEXT_MINOR_VERSION);

    //glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 8);

    if (version >= 32) {
        // OpenGL 3.2+ allow specification of profile

        GLuint profile;
        if (FLEXT_CORE_PROFILE) {
            profile = GLFW_OPENGL_CORE_PROFILE;
        } else {
            profile = GLFW_OPENGL_COMPAT_PROFILE;
        }

        //glfwOpenWindowHint(GLFW_OPENGL_PROFILE, profile);
    }

    // Create window and OpenGL context
    GLint success = glfwOpenWindow(size.x, size.y, 0,0,0,0, 24, 8, GLFW_WINDOW);

    glfwSwapInterval(0);

    if (!success) {
        cerr << "Failed to create OpenGL window." << endl;
        return GL_FALSE;
    }
    
    // Call flext's init function.
    success = flextInit();

    if (!success) {
        glfwTerminate();
    }

    return success;
}
