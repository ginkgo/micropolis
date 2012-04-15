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
