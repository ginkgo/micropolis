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

    uint64_t _last_bound_n_split;
    uint64_t _total_bound_n_split;

    public:

    float  frames_per_second;
    float  ms_per_frame;
    float  ms_per_render_pass;
    float  ms_bound_n_split;
    int    patches_per_frame;
    uint64_t   opencl_memory;
    
    public:
        
    Statistics();

    void start_render();
    void end_render();

    void start_bound_n_split();
    void stop_bound_n_split();

    void inc_patch_count();

    void alloc_opencl_memory(long mem_size);
    void free_opencl_memory(long mem_size);

    void update();
    void reset_timer();

    void print();
        
};

extern Statistics statistics;

#endif
