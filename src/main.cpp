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


void mainloop(GLFWwindow* window)
{
    CL::Device device(config.platform_id(), config.device_id());

    if (config.verbose()) {
        device.print_info();
    }

    Reyes::Scene scene(new Reyes::PerspectiveProjection(75.0f, 0.01f, config.window_size()));
    scene.add_patches(config.input_file());

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

        if (glfwGetKey(window, GLFW_KEY_UP)) {
            view = glm::translate<float>(0,0,-time_diff) * view;
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN)) {
            view = glm::translate<float>(0,0, time_diff) * view;
        }

        view *= glm::rotate<float>((float)time_diff * 5, 0,0,1);
        view *= glm::rotate<float>((float)time_diff * 7, 0,1,0);
        view *= glm::rotate<float>((float)time_diff * 11, 1,0,0);


        scene.set_view(view);

        if (glfwGetKey(window, GLFW_KEY_F3)) {
            scene.draw(wire_renderer);
            statistics.reset_timer();
        } else {
            scene.draw(*renderer);
        }

        statistics.update();


        // Check if the window has been closed
        running = running && !glfwGetKey( window, GLFW_KEY_ESCAPE );
        running = running && !glfwGetKey( window,  'Q' );
		running = running && !glfwWindowShouldClose( window );
		
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

GLFWwindow* init_opengl(ivec2 window_size);

int main(int argc, char** argv)
{
    handle_arguments(argc, argv);

    ivec2 size = config.window_size();

	GLFWwindow* window = init_opengl(size);
	
    if (window == NULL) {
        return 1;
    }
    
    cout << endl;
    cout << "MICROPOLIS - A micropolygon rasterizer" << " (c) Thomas Weber 2012" << endl;
    cout << endl;

    glfwSetWindowTitle(window, config.window_title().c_str());

    try {

        mainloop(window);

    } catch (CL::Exception& e) {

        cerr << e.file() << ":" << e.line_no() << ": error: " <<  e.msg() << endl;

    }

    // glfwTerminate();
    return 0;
}

/* 
 * Helper function that properly initializes the GLFW window before opening.
 */
GLFWwindow* init_opengl(ivec2 size)
{
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return GL_FALSE;
    }

    int version = FLEXT_MAJOR_VERSION * 10 + FLEXT_MINOR_VERSION;


    // Create window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(size.x, size.y, "Window title", NULL, NULL);

	if (!window) {
        cerr << "Failed to create OpenGL window." << endl;
		return NULL;
	}
	
	glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    // Call flext's init function.
    int success = flextInit(window);

    if (!success) {
        glfwTerminate();
    }

    return window;
}
