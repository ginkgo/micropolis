#pragma once

#include "common.h"


namespace GL
{

    class Buffer
    {
        GLuint _buffer;
        size_t _size;

        mutable GLenum _target;
        mutable int _index;
        
    public:

        Buffer(size_t size);
        ~Buffer();

        GLuint get_id() const;
        size_t get_size() const;

        GLuint get_target_index() const;

        void bind(GLenum target) const;
        void bind(GLenum target, GLuint index) const;

        void unbind() const;
        
        void send_data(void* data, size_t size);
        void send_subdata(void* data, size_t offset, size_t size);
        
    };

}
