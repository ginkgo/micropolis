
#include "common.h"

#include "Patch.h"
#include "Config.h"

#include "opengl_draw.h"

#include "Shader.h"
#include "Texture.h"
#include "Image.h"

#include "OpenCL.h"

int openWindow( int width, int height, 
                int redbits, int greenbits, int bluebits, int alphabits,
                int depthbits, int stencilbits, int mode );

void make_checkers(Image& image)
{
    vec4 *pixels = (vec4*)image.data();

    int n = 2;
    float in = 1/float(n);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int pos = x + y * image.width();
            
            vec4 c(0.f,0.f,0.f,1.f);

            for (int i = 0; i < n; ++i) {
                if (((x>>i+3)+(y>>i+3))%2 == 1) {
                    c = c + vec4(in,in,in,0.f);
                }
            }

            pixels[pos] = c;
        }
    }
}

static bool close_window = false;
static int GLFWCALL window_close_callback( void )
{
    close_window = true;
    return GL_FALSE;
}

void opencl_main(vector<BezierPatch>& patches)
{

    glfwSetWindowCloseCallback(window_close_callback);

    try {
        OpenCL::Device device(config.platform_id(), config.device_id());

        device.print_info();
    
        OpenCL::Kernel kernel(device, "test.cl", "test");

        ivec2 w_size = config.window_size();
        Image *image = new Image(2, w_size.x, w_size.y, 0,
                                 GL_RGBA, GL_FLOAT, sizeof(vec4));

        make_checkers(*image);

        Texture framebuffer(*image, GL_NEAREST, GL_NEAREST);
        delete image;

        OpenCL::ImageBuffer tex_buffer(device, framebuffer, CL_MEM_WRITE_ONLY);
        OpenCL::CommandQueue queue(device);
    
        Shader shader("tex_draw");

        bool running = true;

        while (running) {
            // OpenCL::Event event;
        
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            queue.enq_GL_acquire(tex_buffer);
        
            kernel.set_arg_r(0, tex_buffer);
            kernel.set_arg(1, (float)glfwGetTime());
            queue.enq_kernel(kernel, config.window_size(), ivec2(16, 16));
        
            queue.enq_GL_release(tex_buffer);
            queue.finish();

            // queue.enq_GL_release(tex_buffer, event);
            // OpenCL::sync_GL(event);

            framebuffer.bind();
        
            shader.bind();

            shader.set_uniform("framebuffer", framebuffer);

            glBegin(GL_QUADS);
            glVertex2f(-1,-1);
            glVertex2f( 1,-1);
            glVertex2f( 1, 1);
            glVertex2f(-1, 1);
            glEnd();

            shader.unbind();
        
            framebuffer.unbind();

            glfwSwapBuffers();

            float fps, mspf;
            calc_fps(fps, mspf);

            // Check if the window has been closed
            running = running && !glfwGetKey( GLFW_KEY_ESC );
            running = running && !glfwGetKey( 'Q' );
            running = running && !close_window;
        }   
    } catch (OpenCL::Exception& e) {
        cerr << "OpenCL error (" << e.file() << ":" << e.line_no()
             << "): " <<  e.msg() << endl;
    }                     
}

int main(int argc, char** argv)
{
    if (!Config::load_file("options.txt", config)) {
        cout << "Failed to load options.txt" << endl;
    }

    argc = config.parse_args(argc, argv);

    if (!Config::save_file("options.txt", config)) {
        cout << "Failed to save options.txt" << endl;
    }

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " file" << endl;
        return 0;            
    }

    ivec2 size = config.window_size();

    vector<BezierPatch> patches;
    read_patches(argv[1], patches);

    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return 1;
    }

    if (!openWindow(size.x, size.y, 0,0,0,0,0,0, GLFW_WINDOW)) {
        glfwTerminate();
        return 1;
    }

    ogl_main(patches);
    //opencl_main(patches);

    glfwCloseWindow();
    glfwTerminate();
    return 0;
}

/* 
 * Helper function that properly initializes the GLFW window before opening.
 */
int openWindow( int width, int height, 
                int redbits, int greenbits, int bluebits, int alphabits,
                int depthbits, int stencilbits, int mode )
{
    int version = FLEXT_MAJOR_VERSION * 10 + FLEXT_MINOR_VERSION;

    // We can use this to setup the desired OpenGL version in GLFW
    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, FLEXT_MAJOR_VERSION);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, FLEXT_MINOR_VERSION);

    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 8);

    if (version >= 32) {
        // OpenGL 3.2+ allow specification of profile

        GLuint profile;
        if (FLEXT_CORE_PROFILE) {
            profile = GLFW_OPENGL_CORE_PROFILE;
        } else {
            profile = GLFW_OPENGL_COMPAT_PROFILE;
        }

        glfwOpenWindowHint(GLFW_OPENGL_PROFILE, profile);
    }

    // Create window and OpenGL context
    GLint success = glfwOpenWindow(width, height,
                                   redbits, greenbits, bluebits, alphabits,
                                   depthbits, stencilbits, mode);

    if (!success) {
        cerr << "Failed to create OpenGL window." << endl;
        return GL_FALSE;
    }
    
    // Call flext's init function.
    success = flextInit();

    return success;
}
