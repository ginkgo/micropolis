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


#ifndef STATISTICS_H
#define STATISTICS_H

#include "common.h"

class Statistics
{
    uint64_t _last_fps_calculation;
    uint64_t _render_start_time;
    int _frames;
    int _patches_per_frame;
    size_t   _bound_count;

    uint64_t _last_bound_n_split;
    uint64_t _total_bound_n_split;
    uint64_t _total_dice_n_raster;
    
    uint64_t _pass_count;

    std::vector<int> _bound_n_split_balance;
    
    public:

    float    frames_per_second;
    float    ms_per_frame;
    float    ms_per_render_pass;
    float    ms_bound_n_split;
    float    ms_dice_n_raster;
    int      patches_per_frame;
    uint64_t opencl_memory;
    uint64_t opengl_memory;
    size_t   max_patches;
    size_t   bounds_per_frame;
    size_t   total_input_patches;

    map<string, uint64_t> opencl_memory_by_use;
    
    public:
        
    Statistics();

    void start_render();
    void end_render();

    void start_bound_n_split();
    void stop_bound_n_split();
    
    void add_bound_n_split_time(uint64_t ns);
    void add_dice_n_raster_time(uint64_t ns);

    void inc_patch_count();
    void add_patches(size_t patches);
    
    void add_bounds(size_t bounds);

    void inc_pass_count(uint64_t cnt);
    uint64_t get_pass_count() { return _pass_count; }

    void alloc_opencl_memory(long mem_size, const string& use);
    void free_opencl_memory(long mem_size, const string& use);

    void alloc_opengl_memory(long mem_size);
    void free_opengl_memory(long mem_size);

    void update_max_patches(size_t current_patches);
    void set_bound_n_split_balance(int* processed, size_t work_group_cnt);

    void set_total_input_patches(size_t tip) { total_input_patches = tip; }
    
    void update();
    void reset_timer();

    void print();
    void dump_stats();
        
};

extern Statistics statistics;

#endif
