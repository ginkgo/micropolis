#include "utility.h"

#include <fstream>

void calc_fps(float* fps, float* mspf)
{
    static double last = -1.0;
    static int frames = 0;
    static float current_fps = .0f;
    static float current_ms_per_frame = .0f;

    double now = glfwGetTime();
  
    if (last < 0.0) {
        last = now;
    }

    frames += 1;

    if (now - last >= 5.0) {
        printf("%.2f ms/frame (= %d fps)\n", 
              ((now-last)*1000.0/frames),
              (int)(frames/(now-last)));
        current_fps = float( frames/(now - last) );
        current_ms_per_frame = float( (now-last)*5000.0/frames );
        last = now;
        frames = 0;
    }

    if (fps != NULL) {
        *fps = current_fps;
    }

    if (mspf != NULL) {
        *mspf = current_ms_per_frame;
    }
}

bool file_exists(const string &filename)
{
    std::ifstream ifile(filename.c_str());
    return ifile.good();
}

string read_file(const string &filename)
{
    std::ifstream ifile(filename.c_str());

    return string(std::istreambuf_iterator<char>(ifile),
        std::istreambuf_iterator<char>());
}


void get_errors(void)
{
    static int call_count = 0;

    // We don't need get_errors if we use ARB_debug_output
    if (true) {
        GLenum error = glGetError();

        if (error != GL_NO_ERROR) {
            if (call_count == 16) return;

            switch (error) {
            case GL_INVALID_ENUM:
                cerr << "GL: enum argument out of range." << endl;
                break;
            case GL_INVALID_VALUE:
                cerr << "GL: Numeric argument out of range." << endl;
                break;
            case GL_INVALID_OPERATION:
                cerr << "GL: Operation illegal in current state." << endl;
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                cerr << "GL: Framebuffer object is not complete." << endl;
                break;
            case GL_OUT_OF_MEMORY:
                cerr << "GL: Not enough memory left to execute command." << endl;
                break;
            default:
                cerr << "GL: Unknown error." << endl;
            }

            ++call_count;
        }
    }
}
