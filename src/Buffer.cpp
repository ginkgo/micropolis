#include "Buffer.h"

#include "Statistics.h"

GL::Buffer::Buffer(size_t size)
    : _buffer(0)
    , _size(size)
    , _target(GL_FALSE)
    , _index(-1)
{
    glGenBuffers(1, &_buffer);

    glBindBuffer(GL_ARRAY_BUFFER, _buffer);
    glBufferData(GL_ARRAY_BUFFER, _size, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    statistics.alloc_opengl_memory(_size);
}


GL::Buffer::~Buffer()
{
    glDeleteBuffers(1, &_buffer);

    statistics.free_opengl_memory(_size);
}


GL::Buffer::Buffer(Buffer&& other)
    : _buffer(other._buffer)
    , _size(other._size)
    , _target(other._target)
    , _index(other._index)
{
    other._buffer = 0;
    other._size = 0;
    other._target = GL_FALSE;
    other._index = -1;
}

GL::Buffer& GL::Buffer::operator= (Buffer&& other)
{
    _buffer = other._buffer;
    _size = other._size;
    _target = other._target;
    _index = other._index;

    other._buffer = 0;
    other._size = 0;
    other._target = GL_FALSE;
    other._index = -1;

    return *this;
}


GLuint GL::Buffer::get_id() const
{
    return _buffer;
}


size_t GL::Buffer::get_size() const
{
    return _size;
}

GLuint GL::Buffer::get_target_index() const
{
    assert(_index >= 0);
    
    return (GLuint)_index;
}


void GL::Buffer::bind(GLenum target) const
{
    assert(_target == GL_FALSE);

    glBindBuffer(target, _buffer);

    _target = target;
    _index = -1;
}

void GL::Buffer::bind(GLenum target, GLuint index) const
{
    assert(_target == GL_FALSE);

    glBindBufferBase(target, index, _buffer);

    _target = target;
    _index = index;
}


void GL::Buffer::unbind() const
{
    assert(_target != GL_FALSE);

    if (_index < 0) {
        glBindBuffer(_target, 0);
    } else {
        glBindBufferBase(_target, _index, 0);
    }

    _target = GL_FALSE;
    _index = -1;
}


void GL::Buffer::send_data(void* data, size_t size)
{
    assert(_target != GL_FALSE);
    glBufferData(_target, size, data, GL_DYNAMIC_COPY);

    _size = size;
}


void GL::Buffer::send_subdata(void* data, size_t offset, size_t size)
{
    assert(_target != GL_FALSE);
    assert(offset+size <= _size);
    glBufferSubData(_target, offset, size, data);    
}


void GL::Buffer::read_data(void* data, size_t size)
{
    assert(size <= _size);

    glGetBufferSubData(_target, 0, size, data);
}
