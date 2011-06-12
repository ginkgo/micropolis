
#include "common.h"

#include "Patch.h"
#include "Config.h"

#include "opengl_draw.h"

#include "Shader.h"
#include "Texture.h"
#include "Image.h"

#include "OpenCL.h"

class Framebuffer
{
    ivec2 _size;
    int _bsize;
    ivec2 _grid_size;
    ivec2 _act_size;
    
    TextureBuffer _tex_buffer;
    OpenCL::Buffer _cl_buffer;

    public:

    Framebuffer(ivec2 size, int bsize, OpenCL::Device& device);
    

    cl_mem get_buffer() { return _cl_buffer.get(); }
    TextureBuffer& get_texture() { return _tex_buffer; }

    ivec2 get_size() { return _size; }
    int get_bsize() { return _bsize; }
    ivec2 get_gridsize() {return _grid_size; }
};

Framebuffer::Framebuffer(ivec2 size, int bsize, OpenCL::Device& device) :
    _size(size), _bsize(bsize), 
    _grid_size(ceil((float)size.x/bsize), ceil((float)size.y/bsize)),
    _act_size(_grid_size * bsize), 
    _tex_buffer(_act_size.x * _act_size.y * sizeof(vec4), GL_RGBA32F),
    _cl_buffer(device, _tex_buffer.get_buffer())
{

}

class OCLPatchDrawer : public PatchDrawer
{
    OpenCL::Device& _device;
    OpenCL::CommandQueue& _queue;
    Framebuffer& _framebuffer;
    Projection& _projection;
    size_t _kernel_width;
    size_t _patches_per_pass;

    vector<vec4> _control_points;
    size_t _patch_cnt;

    mat4 _proj;

    OpenCL::Buffer   _patch_buffer;
    OpenCL::Kernel _patch_kernel;

    public: 

    OCLPatchDrawer(OpenCL::Device&       device,
                   OpenCL::CommandQueue& queue,
                   Framebuffer& framebuffer,
                   Projection& projection,
                   size_t kernel_width,
                   size_t patches_per_pass);

    void prepare();
    void draw_patch(const BezierPatch& patch);
    void flush();

};

void OCLPatchDrawer::prepare()
{
    _patch_cnt = 0;

    mat4 proj;
    _projection.calc_projection(proj);

    _patch_kernel.set_arg_r(4, proj);
    _patch_kernel.set_arg(5, _projection.get_viewport());
}

void OCLPatchDrawer::draw_patch(const BezierPatch& patch)
{
    size_t cp_index = _patch_cnt * 16;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            _control_points[cp_index] = vec4(patch.P[i][j], 1);
            ++cp_index;
        }
    }

    _patch_cnt++;

    if (_patch_cnt >= _patches_per_pass) {
        flush();
    }
}

void OCLPatchDrawer::flush()
{
    if (_patch_cnt == 0) {
        return;
    }

    _queue.enq_write_buffer(_patch_buffer, (void*)_control_points.data(), 
                            sizeof(vec4) * _patch_cnt * 16);

    _patch_kernel.set_arg_r(0, _patch_buffer);
    _patch_kernel.set_arg(1, _framebuffer.get_buffer());
    _patch_kernel.set_arg(2, _framebuffer.get_bsize());
    _patch_kernel.set_arg(3, _framebuffer.get_gridsize());

    _queue.enq_kernel(_patch_kernel, 
                      ivec3(_kernel_width, _kernel_width, _patch_cnt),
                      ivec3(32, 32, 1));

    // TODO:
    // Wait for kernel to finish to avoid overwriting the buffer while it's 
    // copying to gpu. This needs to be optimized later on.
    
    _queue.finish();

    _patch_cnt = 0;
}

OCLPatchDrawer::OCLPatchDrawer(OpenCL::Device&       device,
                               OpenCL::CommandQueue& queue,
                               Framebuffer&  framebuffer,
                               Projection& projection,
                               size_t kernel_width,
                               size_t patches_per_pass) :
    _device(device), _queue(queue), 
    _framebuffer(framebuffer), _projection(projection),
    _kernel_width(kernel_width), _patches_per_pass(patches_per_pass),
    _control_points(patches_per_pass * 16),
    _patch_cnt(0),
    _patch_buffer(_device, patches_per_pass * 16 * sizeof(vec4), CL_MEM_READ_ONLY),
    _patch_kernel(_device, "reyes.cl", "dice")
{

};

int openWindow( int width, int height, 
                int redbits, int greenbits, int bluebits, int alphabits,
                int depthbits, int stencilbits, int mode );


static bool close_window = false;
static int GLFWCALL window_close_callback( void )
{
    close_window = true;
    return GL_FALSE;
}

void opencl_main(vector<BezierPatch>& patches)
{

    glfwSetWindowCloseCallback(window_close_callback);

    
    ivec2 w_size = config.window_size();
    PerspectiveProjection projection(60, 0.01, w_size);

    float distance = 8;


    try {
        OpenCL::Device device(config.platform_id(), config.device_id());

        device.print_info();
    

        Framebuffer framebuffer(w_size, 64, device);

        OpenCL::CommandQueue queue(device);
    
        OCLPatchDrawer patch_drawer(device, queue, framebuffer, 
                                    projection, 128, 64);

        Shader shader("tex_draw");


        OpenCL::Kernel clear_kernel(device, "reyes.cl", "clear");
        clear_kernel.set_arg(0, framebuffer.get_buffer());
        clear_kernel.set_arg(1, vec4(0.1f,0.1f,1,
                                     std::numeric_limits<float>::infinity()));
        clear_kernel.set_arg(2, framebuffer.get_bsize());
        clear_kernel.set_arg(3, framebuffer.get_gridsize());

        bool running = true;

        double last = glfwGetTime();
        double time_diff = 0;

        while (running) {

            double now = glfwGetTime();
            time_diff = now - last;
            last = now;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            queue.enq_GL_acquire(framebuffer.get_buffer());
        
            queue.enq_kernel(clear_kernel, config.window_size(), ivec2(16, 16));
        
            mat4 view;
            view *= glm::translate<float>(0,-2,-distance);
            view *= glm::rotate<float>(-70, 1,0,0);
            view *= glm::rotate<float>(now * 10, 0,0,1);

            mat4x3 view4x3(view);
            BezierPatch transformed;

            patch_drawer.prepare();

            for (size_t i = 0; i < patches.size(); ++i) {
                transform_patch(patches[i], view4x3, transformed);

                split_n_draw(transformed, projection, patch_drawer);
            }
            
            patch_drawer.flush();

            queue.enq_GL_release(framebuffer.get_buffer());

            queue.finish();

            framebuffer.get_texture().bind();
        
            shader.bind();

            shader.set_uniform("framebuffer", framebuffer.get_texture());
            shader.set_uniform("bsize", framebuffer.get_bsize());
            shader.set_uniform("gridsize", framebuffer.get_gridsize());

            glBegin(GL_QUADS);
            glVertex2f(-1,-1);
            glVertex2f( 1,-1);
            glVertex2f( 1, 1);
            glVertex2f(-1, 1);
            glEnd();

            shader.unbind();
        
            framebuffer.get_texture().unbind();

            glfwSwapBuffers();

            float fps, mspf;
            calc_fps(fps, mspf);

            if (glfwGetKey( GLFW_KEY_UP )) {
                distance += time_diff;
            }

            if (glfwGetKey( GLFW_KEY_DOWN )) {
                distance -= time_diff;
            }

            // Check if the window has been closed
            running = running && !glfwGetKey( GLFW_KEY_ESC );
            running = running && !glfwGetKey( 'Q' );
            running = running && !close_window;
        }   
    } catch (OpenCL::Exception& e) {
        cerr << "OpenCL error (" << e.file() << ":" << e.line_no()
             << "): " <<  e.msg() << endl;
    }                     

    glfwCloseWindow();
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

    //ogl_main(patches);
    opencl_main(patches);

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
