#pragma once

#include "common.h"
#include <CL/opencl.h>


namespace CL
{
    class Device;

    class Event
    {
    public:
        
        static const int MAX_ID_COUNT = 16;

    private:
        int _ids[MAX_ID_COUNT];
        size_t _count;

    public:

        Event();
        Event(int id);
        Event(const Event& event);

        Event operator | (const Event& other) const;
        Event& operator = (const Event& other);
        const size_t get_id_count() const;
        const int* get_ids() const;        
    };


    class UserEvent : noncopyable
    {

        Device& _device;
        string _name;
        cl_event _event;
        int _id;
        bool _active;
        
    public:


        UserEvent(Device& device, const string& name);
        ~UserEvent();

        void begin(const CL::Event& dependencies);
        void end();
        
        Event event() const { return Event(_id); }
        
    };

}
