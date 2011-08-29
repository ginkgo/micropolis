#ifndef STATISTICS_H
#define STATISTICS_H

namespace Reyes
{

    class Statistics
    {
        long _last_fps_calculation;
        long _render_start_time;
        int _frames;
        int _patches_per_frame;

        public:

        float frames_per_second;
        float ms_per_frame;
        float ms_per_render_pass;
        int patches_per_frame;

        public:
        
        Statistics();

        void start_render();
        void end_render();

        void inc_patch_count();

        void update();

        void print();
        
    };

}

#endif
