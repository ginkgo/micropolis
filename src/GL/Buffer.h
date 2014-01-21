#pragma once

#include "common.h"
#include <initializer_list>

namespace GL
{

    class Buffer : public noncopyable
    {
        GLuint _buffer;
        size_t _size;

        mutable GLenum _target;
        mutable int _index;
        
    public:

        Buffer(size_t size);
        ~Buffer();

        Buffer(Buffer&& other);
        Buffer& operator=(Buffer&& other);

        GLuint get_id() const;
        size_t get_size() const;

        void resize(size_t new_size);
        
        GLuint get_target_index() const;

        void bind(GLenum target) const;
        void bind(GLenum target, GLuint index) const;

        void unbind() const;
        
        void send_data(void* data, size_t size);
        void send_subdata(void* data, size_t offset, size_t size);

        void read_data(void* data, size_t size);

        
        template<typename ... Types>
        static void bind_all(GLenum target, GLuint start, Buffer& buffer, Types&& ... rest)
        {
            buffer.bind(target, start);
            bind_all(target, start+1, rest...);
        }
        
        template<typename ... Types>
        static void unbind_all(Buffer& buffer, Types&& ... rest)
        {
            buffer.unbind();
            unbind_all(rest...);
        }
        
        static void bind_all(GLenum target, GLuint start) {};
        static void unbind_all() {};        
    };

}
