#pragma once

#include "common.h"
#include <CL/opencl.h>

#include "Event.h"

namespace CL
{
    class Device : public noncopyable
    {

        
        cl_context   _context;
        cl_device_id _device;

        bool _share_gl;

        
    public:

        
        Device(int platform_index, int device_index);
        ~Device();

        cl_device_id get_device()  { return _device; }
        cl_context   get_context() { return _context; }

        bool share_gl() const { return _share_gl; }

        void print_info();

        
    private:

        
        struct EventIndex
        {
            int id;
            string name;
            string queue_name;
            cl_event event;
            
            bool is_user;
            cl_ulong user_begin;
            cl_ulong user_end;

            int dependency_count;
            int dependency_ids[Event::MAX_ID_COUNT];
        };

        std::unordered_map<int, EventIndex> _events;
        int _id_count;
        bool _dump_trace;

        
    public:

        
        Event insert_event(const string& name, const string& queue_name, cl_event event, const Event& dependencies);
        size_t setup_event_pad(const Event& event, vector<cl_event>& event_pad, cl_event*& event_pad_ptr);

        int insert_user_event(const string& name, cl_event event);
        void end_user_event(int id);
        
        void dump_trace();
        void release_events();

        
    };
}
