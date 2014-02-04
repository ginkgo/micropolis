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


#include "Statistics.h"

#include "utility.h"
#include "Config.h"

#include <fstream>
    
Statistics statistics;

Statistics::Statistics()
    : _frames(0)
    , frames_per_second(0.0f)
    , ms_per_frame(0.0f)
    , ms_per_render_pass(0.0f)
    , patches_per_frame(0)
    , opencl_memory(0)
    , opengl_memory(0)
{
    _last_fps_calculation = nanotime();
}

void Statistics::start_render()
{
    _render_start_time = nanotime();
    _patches_per_frame = 0;
    _total_bound_n_split = 0;
    _total_dice_n_raster = 0;
    _pass_count = 0;
}

void Statistics::end_render()
{
    uint64_t dur = nanotime() - _render_start_time;

    ms_per_render_pass = dur * 0.000001f;
    ms_bound_n_split = _total_bound_n_split * 0.000001f;
    ms_dice_n_raster = _total_dice_n_raster * 0.000001f;
    patches_per_frame = _patches_per_frame;
}

void Statistics::inc_patch_count()
{
    ++_patches_per_frame;
}

void Statistics::add_patches(size_t patches)
{
    _patches_per_frame += patches;
}

void Statistics::inc_pass_count(uint64_t cnt)
{
    _pass_count += cnt;
}

void Statistics::start_bound_n_split()
{
    _last_bound_n_split = nanotime();
}

void Statistics::stop_bound_n_split()
{
    uint64_t now = nanotime();
    uint64_t duration = now - _last_bound_n_split;

    _total_bound_n_split += duration;    
}

void Statistics::add_bound_n_split_time(uint64_t ns)
{
    _total_bound_n_split += ns;
}

void Statistics::add_dice_n_raster_time(uint64_t ns)
{
    _total_dice_n_raster += ns;
}

void Statistics::alloc_opencl_memory(long mem_size)
{
    opencl_memory += mem_size;
}

void Statistics::free_opencl_memory(long mem_size)
{
    opencl_memory -= mem_size;
}

void Statistics::alloc_opengl_memory(long mem_size)
{
    opengl_memory += mem_size;
}

void Statistics::free_opengl_memory(long mem_size)
{
    opengl_memory -= mem_size;
}

void Statistics::update()
{
    ++_frames;
    uint64_t now = nanotime();
    uint64_t dur = now - _last_fps_calculation;

    if (dur > 1 * BILLION) {
        frames_per_second = (float)_frames * BILLION / dur;
        ms_per_frame = dur / ((float)_frames * MILLION);

        _last_fps_calculation = now;
        _frames = 0;

        print();
    }
}

void Statistics::reset_timer()
{
    _last_fps_calculation = nanotime();
    _frames = 0;
}

void Statistics::print()
{
    if (config.verbosity_level() > 0) {

        uint64_t quad_count = square(config.reyes_patch_size()) * patches_per_frame;
        double quads_per_second = quad_count * frames_per_second;
        
        cout << endl
             << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl
             // << ms_per_render_pass << " ms/render pass" << endl
             // << ms_bound_n_split << " ms spent on bound & split" << endl
             // << ms_dice_n_raster << " ms spent on dicing & rasterization" << endl
             << patches_per_frame  << " bounded patches" << endl
             << _pass_count << " render passes" << endl
             // << with_commas(quad_count) << " polygons" << endl
             // << with_commas((uint64_t)quads_per_second) << " polys/s" << endl
             << memory_size(opencl_memory) << "allocated on OpenCL device" << endl
             << memory_size(opengl_memory) << "allocated in OpenGL context" << endl;
    } else {
        cout  << ms_per_frame << " ms/frame, (" << frames_per_second  << " fps)" << endl;
    }
}


void Statistics::dump_stats()
{
    std::ofstream fs(config.statistics_file().c_str());

    fs << "opencl_mem = " << opencl_memory << ";" << endl;
    fs << "opengl_mem = " << opengl_memory << ";" << endl;
    fs << "patches_per_frame = " << patches_per_frame << ";" << endl;
    fs << "pass_count = " << _pass_count << ";" << endl;
    
}
