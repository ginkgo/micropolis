#include "Event.h"


CL::Event::Event() :
    _count(0)
{

}

CL::Event::Event(long id) :
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

const long* CL::Event::get_ids() const
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
