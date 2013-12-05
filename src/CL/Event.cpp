#include "Event.h"

#include "Device.h"
#include "Exception.h"

CL::Event::Event() :
    _count(0)
{

}

CL::Event::Event(int id) :
    _count(1)
{
    _ids[0] = id;
}

CL::Event::Event(const Event& event) :
    _count(event._count)
{
    assert(event._count < MAX_ID_COUNT);

    for (size_t i = 0; i < _count; ++i) {
        _ids[i] = event._ids[i];
    }
}

const size_t CL::Event::get_id_count() const
{
    return _count;
}

const int* CL::Event::get_ids() const
{
    return _ids;
}

CL::Event CL::Event::operator | (const CL::Event& other) const
{
    Event e(other);

    e._count += _count;

    assert(e._count < MAX_ID_COUNT);

    for (size_t i = 0; i < _count; ++i) {
        e._ids[i + other._count] = _ids[i];
    }

    return e;
}

CL::Event& CL::Event::operator = (const CL::Event& other)
{
    _count = other._count;

    for (size_t i = 0; i < _count; ++i) {
        _ids[i] = other._ids[i];
    }

    return *this;
}




CL::UserEvent::UserEvent(CL::Device& device, const string& name)
    : _device(device)
    , _name(name)
    , _event(0)
    , _id(-1)
    , _active(false)
{
}


CL::UserEvent::~UserEvent()
{
    if (_active) {
        end();
    }
}


void CL::UserEvent::begin()
{
    cl_int status;
    
    _event = clCreateUserEvent(_device.get_context(), &status);
    OPENCL_ASSERT(status);

    _id = _device.insert_user_event(_name, _event);
    _active = true;
}


void CL::UserEvent::end()
{
    cl_int status;
    
    if (!_active) return;

    status = clSetUserEventStatus(_event, CL_COMPLETE);
    OPENCL_ASSERT(status);

    _device.end_user_event(_id);
    
    _event = 0;
    _id = -1;
    _active = false;
}
