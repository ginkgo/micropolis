#include "Statistics.h"

#include "utility.h"
#include "Config.h"
    
Statistics statistics;

Statistics::Statistics() :
    _frames(0), 
    frames_per_second(0.0f),
    ms_per_frame(0.0f),
    ms_per_render_pass(0.0f),
    patches_per_frame(0),
    opencl_memory(0)
{
    _last_fps_calculation = nanotime();
}

void Statistics::start_render()
{
    _render_start_time = nanotime();
    _patches_per_frame = 0;
}

void Statistics::inc_patch_count()
{
    ++_patches_per_frame;
}

void Statistics::end_render()
{
    long dur = nanotime() - _render_start_time;

    ms_per_render_pass = dur * 0.000001f;
    patches_per_frame = _patches_per_frame;
}

void Statistics::alloc_opencl_memory(long mem_size)
{
    opencl_memory += mem_size;
}

void Statistics::free_opencl_memory(long mem_size)
{
    opencl_memory -= mem_size;
}

void Statistics::update()
{
    ++_frames;
    long now = nanotime();
    long dur = now - _last_fps_calculation;

    if (dur > 5 * BILLION) {
        frames_per_second = (float)_frames * BILLION / dur;
        ms_per_frame = dur / ((float)_frames * MILLION);

        _last_fps_calculation = now;
        _frames = 0;

        print();
    }
}

void Statistics::print()
{
    if (config.verbose()) {
        cout << endl
             << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl
             << ms_per_render_pass << " ms/render pass" << endl
             << patches_per_frame  << " bounded patches/frame" << endl
             << (opencl_memory >> MEBI_SHIFT) << " MiB allocated on OpenCL device" << endl;
    } else {
        cout  << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl;
    }
}
