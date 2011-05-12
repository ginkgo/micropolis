#include "utility.h"

#include <fstream>

void calc_fps(float& fps, float& mspf)
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

    fps = current_fps;
    mspf = current_ms_per_frame;
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
