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

#include "CL/HistogramPyramid.h"
#include "CL/OpenCL.h"
#include "CL/PrefixSum.h"
#include "Config.h"
#include "CLConfig.h"
#include "GLConfig.h"
#include "ReyesConfig.h"
#include "GL/Buffer.h"
#include "GL/PrefixSum.h"
#include "Reyes/Reyes.h"
#include "Statistics.h"
#include "Scene.h"

#include <boost/format.hpp>
#include <algorithm>
#include <random>

void mainloop(GLFWwindow* window);
bool test_GL_prefix_sum(const int N, bool print);
bool test_CL_prefix_sum(const int N, bool print);
bool test_CL_histogram_pyramid(const int N, bool print);
bool handle_arguments(int& argc, char** argv);
GLFWwindow* init_opengl(ivec2 window_size);
void get_framebuffer_info();

int main(int argc, char** argv)
{
    // Parse config file and command line arguments
    if (!handle_arguments(argc, argv)) {
        return 1;
    }

    // Check if input file exists
    if (!file_exists(reyes_config.input_file())) {
        cerr << "Input file \"" << reyes_config.input_file() << "\" does not exist." << endl;
        return 1;
    }
    
    ivec2 size = reyes_config.window_size();

	GLFWwindow* window = init_opengl(size);
    
    if (window == NULL) {
        return 1;
    }
    
    cout << endl;
    cout << "MICROPOLIS - A micropolygon rasterizer" << " (c) Thomas Weber 2012" << endl;
    cout << endl;

    glfwSetWindowTitle(window, reyes_config.window_title().c_str());

    try {

        mainloop(window);

    } catch (CL::Exception& e) {
        cerr << e.file() << ":" << e.line_no() << ": error: " <<  e.msg() << endl;
        return 1;
    }

    return 0;
}

void mainloop(GLFWwindow* window)
{

    Reyes::Scene scene(reyes_config.input_file());
    
    shared_ptr<Reyes::Renderer> renderer;
    
    switch (reyes_config.renderer_type()) {
    case ReyesConfig::OPENCL:
        renderer.reset(new Reyes::RendererCL());
        break;
    // case ReyesConfig::GLTESS:
    //     renderer.reset(new Reyes::RendererGLHWTess());
    //     break;
    default:
        cout << "Unimplemented renderer" << endl;
        return;
    }

    bool running = true;

    statistics.reset_timer();
    double last = glfwGetTime();

    glm::dvec2 last_cursor_pos;
    glfwGetCursorPos(window, &(last_cursor_pos.x), &(last_cursor_pos.y));
    
    bool in_wire_mode = false;

    Keyboard keys(window);

    // for (auto N : {1,2, 20, 100,
    //             128, 200, 512, 800, 1000,
    //             1024, 2048, 4096, 5000,
    //             128*128, 128*128*128, 1024*1024*128, 50000}) {
    //     if (test_CL_prefix_sum(N, false)) {
    //         cout << format("Prefix sum on %1% items succeeded") % N << endl;
    //     } else {
    //         cout << format("Prefix sum on %1% items failed") % N << endl;
    //     }
    // }    
    // return;
    
    // for (auto N : {1,2, 20, 50,
    //             100, 128, 200, 512, 800, 1000,
    //             1024, 2048, 4096, 5000,
    //              128*128, 128*128*128, 50000
    //             }) {
    //     if (test_CL_histogram_pyramid(N, false)) {
    //         cout << format("Prefix sum on %1% items succeeded") % N << endl;
    //     } else {
    //         cout << format("Prefix sum on %1% items failed") % N << endl;
    //     }
    // }    
    // return;

    long long frame_no = 0;

    string trace_file = cl_config.trace_file();
    string statistics_file = config.statistics_file();
    
    while (running) {

        glfwPollEvents();
        
        double now = glfwGetTime();
        double time_diff = now - last;
        last = now;

        // Camera navigation
        vec3 translation((glfwGetKey(window, 'A') ? -1 : 0) + (glfwGetKey(window, 'D') ? 1 : 0), 0,
                         (glfwGetKey(window, 'W') ? -1 : 0) + (glfwGetKey(window, 'S') ? 1 : 0));
        translation *= time_diff * 2;

        glm::dvec2 cursor_pos;
        glfwGetCursorPos(window, &(cursor_pos.x), &(cursor_pos.y));

        glm::dvec2  mouse_movement = cursor_pos - last_cursor_pos;
        if (abs(mouse_movement.x) > 100 || abs(mouse_movement.y) > 100) {
            mouse_movement = vec2(0,0);
        }
        
        vec2 rotation(0,0);
        float zrotation = 0.0f;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            rotation = mouse_movement * 0.1;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
            zrotation = mouse_movement.x * 0.4f;
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3) == GLFW_PRESS) {
            translation.x += mouse_movement.x * -0.01;
            translation.y += mouse_movement.y * 0.01;
        }
        last_cursor_pos = cursor_pos;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            translation *= 2.0f;
            zrotation   *= 2.0f;
        }
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
            translation *= 0.25f;
            rotation    *= 0.50f;
            zrotation   *= 0.50f;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP)) {
            reyes_config.set_bound_n_split_limit(reyes_config.bound_n_split_limit() * pow(0.75, time_diff));
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN)) {
            reyes_config.set_bound_n_split_limit(reyes_config.bound_n_split_limit() * pow(0.75,-time_diff));
        }
        
        scene.active_cam().transform = scene.active_cam().transform
            * glm::translate<float>(translation.x, translation.y, translation.z)
            * glm::rotate<float>(rotation.x, 0,1,0)
            * glm::rotate<float>(rotation.y, 1,0,0)
            * glm::rotate<float>( zrotation, 0,0,1);

        // Wireframe toggle
        if (keys.pressed(GLFW_KEY_F3)) {
            in_wire_mode = !in_wire_mode;
            statistics.reset_timer();
        }
        
        // Dump trace
        if (keys.pressed(GLFW_KEY_F9)) {
            renderer->dump_trace();
            statistics.dump_stats();
        }

        // Save scene
        if (keys.pressed(GLFW_KEY_F12)) {
            scene.save(reyes_config.input_file(), keys.is_down(GLFW_KEY_LEFT_SHIFT));
        }

        // Make screenshot
        if (keys.pressed(GLFW_KEY_PRINT_SCREEN)) {
            make_screenshot();
        }
        

        if (config.dump_mode() && frame_no >= config.dump_after()) {
            int dump_id = frame_no - config.dump_after();
            
            cl_config.set_trace_file(trace_file + lexical_cast<string>(dump_id));
            config.set_statistics_file(statistics_file + lexical_cast<string>(dump_id));
                
            renderer->dump_trace();
            statistics.dump_stats();
        }
        
        // Render scene
        statistics.start_render();
        if (in_wire_mode) {
            //scene.draw(wire_renderer);
        } else {
            scene.draw(*renderer);
        }
        
        glfwSwapBuffers(window);
        statistics.end_render();

        statistics.update();

        // Check if the window has been closed
        running = running && keys.is_up(GLFW_KEY_ESCAPE);
        running = running && keys.is_up('Q');
		running = running && !glfwWindowShouldClose( window );

        		
        if (config.dump_mode() && frame_no + 1 >= config.dump_after() + config.dump_count()) {
            running = false;
        }

        frame_no++;

        keys.update();
    }
}




bool test_GL_prefix_sum(const int N, bool print)
{
    bool retval = true;
    
    GL::PrefixSum prefix_sum(N);

    GL::Buffer i_buffer(N * sizeof(ivec2));
    GL::Buffer o_buffer(N * sizeof(ivec2));
    GL::Buffer t_buffer(sizeof(ivec2));

    vector<ivec2> i_vec(N);
    vector<ivec2> o_vec(N);

    srand(43);
    if (print) cout << "INPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        i_vec[i] = ivec2(1, rand()%8+1);
        o_vec[i] = ivec2(0, 0);

        if (print) cout << boost::format("(%1%, %2%) ") % i_vec[i].x % i_vec[i].y;
    }
    if (print) cout << endl;

    i_buffer.bind(GL_ARRAY_BUFFER);
    i_buffer.send_subdata(i_vec.data(), 0, N * sizeof(ivec2));
    i_buffer.unbind();

    o_buffer.bind(GL_ARRAY_BUFFER);
    o_buffer.send_subdata(o_vec.data(), 0, N * sizeof(ivec2));
    o_buffer.unbind();

    prefix_sum.apply(N, i_buffer, o_buffer, t_buffer);

    ivec2 total;

    o_buffer.bind(GL_ARRAY_BUFFER);
    o_buffer.read_data(o_vec.data(), N * sizeof(ivec2));
    o_buffer.unbind();
    
    t_buffer.bind(GL_ARRAY_BUFFER);
    t_buffer.read_data(&total, sizeof(ivec2));
    t_buffer.unbind();
    
    ivec2 sum(0);
    if (print) cout << "OUTPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        sum += i_vec[i];

        if (sum != o_vec[i]) {
            retval = false;
        }
        
        if (print) cout << boost::format("(%1%, %2%) ") % o_vec[i].x % o_vec[i].y;
    }
    if (print) cout << endl;

    if (sum != total) {
        retval = false;
    }

    if (print) cout << boost::format("TOTAL: (%1%, %2%)") % total.x % total.y << endl;
    
    return retval;
}

bool test_CL_prefix_sum(const int N, bool print)
{
    bool retval = true;

    CL::Device device(cl_config.opencl_device_id().x, cl_config.opencl_device_id().y);
    CL::CommandQueue queue(device, "prefix sum test");
    
    CL::PrefixSum prefix_sum(device, N);

    CL::Buffer i_buffer(device, N * sizeof(int), CL_MEM_READ_WRITE);
    CL::Buffer o_buffer(device, N * sizeof(int), CL_MEM_READ_WRITE);
    CL::Buffer t_buffer(device, sizeof(int), CL_MEM_READ_WRITE);

    vector<int> i_vec(N);
    vector<int> o_vec(N);

    srand(43);
    if (print) cout << "INPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        i_vec[i] = rand()%8+1;
        o_vec[i] = 0;

        if (print) cout << i_vec[i] << " ";
    }
    if (print) cout << endl;

    int total;

    CL::Event event;

    event = queue.enq_write_buffer(i_buffer, i_vec.data(), i_vec.size() * sizeof(int), "fill input buffer", CL::Event());
    event = prefix_sum.apply(N, queue, i_buffer, o_buffer, t_buffer, event);
    event = queue.enq_read_buffer(o_buffer, o_vec.data(), o_vec.size() * sizeof(int), "read prefix results", event);
    event = queue.enq_read_buffer(t_buffer, &total, sizeof(int), "read prefix total", event);

    queue.wait_for_events(event);

    int sum = 0;
    if (print) cout << "OUTPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        sum += i_vec[i];

        if (sum != o_vec[i]) {
            retval = false;
        }
        
        if (print) cout << o_vec[i] << " ";
    }
    if (print) cout << endl;

    if (sum != total) {
        retval = false;
    }
    
    if (print) cout << boost::format("TOTAL: %1%") % total << endl;
    
    return retval;
}


bool test_CL_histogram_pyramid(const int N, bool print)
{
    std::random_device rd;
    std::default_random_engine reng(rd());
    std::uniform_int_distribution<int> dist(0, 50000);
    
    bool retval = true;

    CL::Device device(cl_config.opencl_device_id().x, cl_config.opencl_device_id().y);
    CL::CommandQueue queue(device, "prefix sum test");
    
    CL::HistogramPyramid histogram_pyramid(device);

    const int P = histogram_pyramid.pyramid_size(N);
    
    CL::Buffer buffer(device, P * sizeof(int), CL_MEM_READ_WRITE);
    
    vector<int> i_vec(N);
    vector<int> pyramid(P);

    size_t top_level = histogram_pyramid.get_top_level(N);

    vector<size_t> offsets, sizes;
    histogram_pyramid.get_offsets(N, offsets);
    histogram_pyramid.get_sizes(N, sizes);
    
    histogram_pyramid.get_offsets(N, offsets);
    
    if (print) cout << "INPUT: "; 
    for (size_t i = 0; i < (size_t)N; ++i) {
        i_vec[i] = dist(reng);
        
        if (print) cout << i_vec[i] << " ";
    }
    if (print) cout << endl;

    int total;

    CL::Event event;

    event = queue.enq_write_buffer(buffer, i_vec.data(), i_vec.size() * sizeof(int), "fill input buffer", CL::Event());
    event = histogram_pyramid.apply(N, N, queue, buffer, event);
    event = queue.enq_read_buffer(buffer, pyramid.data(), pyramid.size() * sizeof(int), "read pyramid", event);
    
    queue.wait_for_events(event);

    if (print) {
        cout << "OUTPUT:" << endl;
        for (size_t level = 0; level <= top_level; ++level) {
            cout << offsets[level] << ": ";
            for (size_t i = 0; i < sizes[level]; ++i) {
                cout << pyramid[offsets[level]+i] << " ";
            }
            cout << endl;
        }
        cout << endl << endl;
    }

    
    for (size_t i = 0; i < i_vec.size(); ++i) {
        if (i_vec[i] != pyramid[i]) {
            retval = false;
        }
    }
    
    for (size_t level = 0; level < top_level; ++level) {
        size_t offset = offsets[level];
        size_t offset2 = offsets[level+1];
        size_t size = sizes[level];

        for (size_t i = 1; i < size; i+=2) {
            if (pyramid[offset2 + i/2] != pyramid[offset + i] + pyramid[offset + i - 1]) {
                cout << "Mismatch at level " << level << ", element " << i << endl;
                retval = false;
            }
        }

        if ((size%2) == 1 && pyramid[offset2 + size/2] != pyramid[offset + size - 1]) {
            retval = false;
        }
    }
    
    return retval;
}


bool handle_arguments(int& argc, char** argv)
{
    bool needs_resave;

    // Config
    
    if (!Config::load_file("micropolis.options", config, needs_resave)) {
        cout << "Failed to load micropolis.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!Config::save_file("micropolis.options", config)) {
            cout << "Failed to save micropolis.options" << endl;
            return false;
        }
    }

    // CLConfig
    
    if (!CLConfig::load_file("cl.options", cl_config, needs_resave)) {
        cout << "Failed to load cl.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!CLConfig::save_file("cl.options", cl_config)) {
            cout << "Failed to save micropolis.options" << endl;
            return false;
        }
    }
    
    // GLConfig
    
    if (!GLConfig::load_file("gl.options", gl_config, needs_resave)) {
        cout << "Failed to load gl.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!GLConfig::save_file("gl.options", gl_config)) {
            cout << "Failed to save micropolis.options" << endl;
            return false;
        }
    }
    
    // ReyesConfig

    if (!ReyesConfig::load_file("reyes.options", reyes_config, needs_resave)) {
        cout << "Failed to load reyes.options" << endl;
        return false;
    }

    if (needs_resave) {
        if (config.verbosity_level() > 0) {
            cout << "Config file out of date. Resaving." << endl;
        }


        if (!ReyesConfig::save_file("reyes.options", reyes_config)) {
            cout << "Failed to save micropolis.options" << endl;
            return false;
        }
    }


    config.parse_args(argc, argv); 
    cl_config.parse_args(argc, argv); 
    gl_config.parse_args(argc, argv); 
    reyes_config.parse_args(argc, argv);

    return true;
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

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, gl_config.fsaa_samples());
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // Create window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(size.x, size.y, "Window title", NULL, NULL);

	if (!window) {
        cerr << "Failed to create OpenGL window." << endl;
		return NULL;
	}

    glfwSetWindowPos(window, 500, 500);
	
	glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    // Call flext's init function.
    int success = flextInit(window);

    if (!success) {
        glfwTerminate();
    }

    set_GL_error_callbacks();
    
    return window;
}


/**
 * Query properties of attached OpenGL Framebuffer
 */
void get_framebuffer_info()
{
    cout << endl << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << " Framebuffer info...                                                            " << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << endl;
    GLint params[10];
    
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, params);
    switch(params[0]) {
    case GL_NONE:
        cout << "Attachment type: NONE" << endl; break;
    case GL_FRAMEBUFFER_DEFAULT:
        cout << "Attachment type: default framebuffer" << endl; break;
    case GL_RENDERBUFFER:
        cout << "Attachment type: renderbuffer" << endl; break;
    case GL_TEXTURE:
        cout << "Attachment type: texture" << endl; break;
    default:
        cout << "Attachment type: unknown" << endl; break;
    }

    std::fill(params, params+10, -1);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, params);
    cout << "Attachment object name: " << params[0] << endl;

    GLint object_name = params[0];
    
    if (glIsRenderbuffer(object_name)) {
        cout << "Object name is a renderbuffer" << endl;
        
        std::fill(params, params+10, -1);
        glBindRenderbuffer(GL_RENDERBUFFER, object_name);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, params+0);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, params+1);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, params+2);

        cout << "Format: " << params[0] << "x" << params[1] << " @ " << params[2] << " samples" << endl;
    } else {
        cout << "Object name NOT a renderbuffer" << endl;
    }

    if (glIsBuffer(object_name)) {
        cout << "Object name is a buffer" << endl;
    } else {
        cout << "Object name NOT a buffer" << endl;
    }

    if (glIsTexture(object_name)) {
        cout << "Object name is a texture" << endl;
    } else {
        cout << "Object name NOT a texture" << endl;
    }

    if (glIsFramebuffer(object_name)) {
        cout << "Object name is a framebuffer" << endl;
    } else {
        cout << "Object name NOT a framebuffer" << endl;
    }

    cout << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << " Framebuffer info... done                                                       " << endl;
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << endl << endl;
}
