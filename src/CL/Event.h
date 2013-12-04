#pragma once

#include "common.h"
#include <CL/opencl.h>


namespace CL
{

    class Event : public noncopyable
    {
        static const int MAX_ID_COUNT = 16;
        
        long _ids[MAX_ID_COUNT];
        size_t _count;

        public:

        Event();
        Event(long id);
        Event(const Event& event);

        Event operator | (const Event& other) const;
        Event& operator = (const Event& other);
        const size_t get_id_count() const;
        const long* get_ids() const;        
    };

    
}
