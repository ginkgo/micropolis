
#include "common.h"

#include "Patch.h"
#include "Config.h"

#include "opengl_draw.h"

#include "OpenCL.h"
#include "Reyes.h"

#include "Statistics.h"

static bool close_window = false;
static int GLFWCALL window_close_callback( void )
{
    close_window = true;
    return GL_FALSE;
}

void mainloop()
{
    Reyes::Statistics statistics;

    CL::Device device(config.platform_id(), config.device_id());

    if (config.verbose()) {
        device.print_info();
        cout << endl;
        cout << "Device is" << (device.share_gl() ? " " : " not ") << "shared." << endl << endl;
    }


    Reyes::Scene scene(new Reyes::PerspectiveProjection(60.0f, 0.01f, config.window_size()));
    scene.add_patches(config.input_file());

    Reyes::OGLSharedFramebuffer framebuffer(device, config.window_size(), 
                                            config.framebuffer_tile_size());
    mat4 view;
    view *= glm::translate<float>(0,-2,-8);
    view *= glm::rotate<float>(-90, 1,0,0);

    CL::CommandQueue queue(device);
    Reyes::Renderer renderer(device, queue, framebuffer, statistics);

    bool running = true;

    double last = glfwGetTime();
    while (running) {

        double now = glfwGetTime();
        double time_diff = now - last;
        last = now;

        view *= glm::rotate<float>(time_diff * 5, 0,0,1);

        scene.set_view(view);
        renderer.prepare();

        scene.draw(renderer);

        renderer.finish();

        framebuffer.show();

        glfwSwapBuffers();
        
        statistics.update();

        // Check if the window has been closed
        running = running && !glfwGetKey( GLFW_KEY_ESC );
        running = running && !glfwGetKey( 'Q' );
        running = running && !close_window;
    }
}

void handle_arguments(int argc, char** argv)
{
    if (!Config::load_file("options.txt", config)) {
        cout << "Failed to load options.txt" << endl;
    }

    if (!Config::save_file("options.txt", config)) {
        cout << "Failed to save options.txt" << endl;
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

    cout << "MICROPOLIS - A micropolygon rasterizer" 
         << " (c) Thomas Weber 2011" << endl;

    glfwSetWindowCloseCallback(window_close_callback);

    try {

        mainloop();

    } catch (CL::Exception& e) {

        cerr << "OpenCL error (" << e.file() << ":" << e.line_no() << "): " <<  e.msg() << endl;

    }

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
