/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


/** @file type_info.h */

#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "common.h"

/**
 * This defines per-type information using a template-struct.
 */
template <typename T> struct gltype_info
{
    // Abstract
};

template<> struct gltype_info<GLshort>
{
    static const GLenum type = GL_SHORT;
    static const GLenum format = GL_RED_INTEGER;
    static const GLenum internal_format = GL_R16I;

    static const GLint components = 1;

    static void set_uniform(GLint location, GLshort value)
    {
        glUniform1i(location, (GLint)value);
    }
};

template<> struct gltype_info<GLint>
{
    static const GLenum type = GL_INT;
    static const GLenum format = GL_RED_INTEGER;
    static const GLenum internal_format = GL_R32I;

    static const GLint components = 1;

    static void set_uniform(GLint location, GLint value)
    {
        glUniform1i(location, value);
    }

    static void set_memory_location(GLint v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((GLint*)location) = v;
    }
};

template <> struct gltype_info<GLfloat>
{
    static const GLenum type = GL_FLOAT;
    static const GLenum format = GL_RED;
    static const GLenum internal_format = GL_R32F;

    static const GLint components = 1;

    static void set_uniform(GLint location, GLfloat value)
    {
        glUniform1f(location, value);
    }

    static void set_float_array(const GLfloat& in, float* v)
    {
        *v = in;
    }

    static GLfloat build_from_floats(const float* v)
    {
        return *v;
    }


    static void set_memory_location(GLfloat v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((GLfloat*)location) = v;
    }
};

template <> struct gltype_info<vec2>
{
    static const GLenum type = GL_FLOAT;
    static const GLenum format = GL_RG;
    static const GLenum internal_format = GL_RG32F;

    static const GLint components = 2;

    static void set_uniform(GLint location, const vec2& value)
    {
        glUniform2fv(location, 1, (GLfloat*)&value);
    }

    static void set_float_array(const vec2& in, float* v)
    {
        v[0] = in.x;
        v[1] = in.y;
    }

    static vec2 build_from_floats(const float* v)
    {
        return vec2(v[0],v[1]);
    }

    static void set_memory_location(const vec2& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((vec2*)location) = v;
    }
};

template <> struct gltype_info<vec3>
{
    static const GLenum type = GL_FLOAT;
    static const GLenum format = GL_RGB;
    static const GLenum internal_format = GL_RGB32F;

    static const GLint components = 3;

    static void set_uniform(GLint location, const vec3& value)
    {
        glUniform3fv(location, 1, (GLfloat*)&value);
    }

    static void set_float_array(const vec3& in, float* v)
    {
        v[0] = in.x;
        v[1] = in.y;
        v[2] = in.z;
    }

    static vec3 build_from_floats(const float* v)
    {
        return vec3(v[0],v[1], v[2]);
    }

    static void set_memory_location(const vec3& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((vec3*)location) = v;
    }
};

template <> struct gltype_info<vec4>
{
    static const GLenum type = GL_FLOAT;
    static const GLenum format = GL_RGBA;
    static const GLenum internal_format = GL_RGBA32F;

    static const GLint components = 4;

    static void set_uniform(GLint location, const vec4& value)
    {
        glUniform4fv(location, 1, (GLfloat*)&value);
    }

    static void set_float_array(const vec4& in, float* v)
    {
        v[0] = in.x;
        v[1] = in.y;
        v[2] = in.z;
        v[3] = in.w;
    }

    static vec4 build_from_floats(const float* v)
    {
        return vec4(v[0],v[1],v[2],v[3]);
    }

    static void set_memory_location(const vec4& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((vec4*)location) = v;
    }
};

template <> struct gltype_info<ivec2>
{
    static const GLenum type = GL_INT;
    static const GLenum format = GL_RGBA;
    static const GLenum internal_format = GL_RGBA32I;

    static const GLint components = 2;

    static void set_uniform(GLint location, const ivec2& value)
    {
        glUniform2iv(location, 1, (GLint*)&value);
    }

    static void set_memory_location(const ivec2& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((ivec2*)location) = v;
    }
};

template <> struct gltype_info<ivec3>
{
    static const GLenum type = GL_INT;
    static const GLenum format = GL_RGBA;
    static const GLenum internal_format = GL_RGBA32I;

    static const GLint components = 3;

    static void set_uniform(GLint location, const ivec3& value)
    {
        glUniform3iv(location, 1, (GLint*)&value);
    }

    static void set_memory_location(const ivec3& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((ivec3*)location) = v;
    }
};

template <> struct gltype_info<ivec4>
{
    static const GLenum type = GL_INT;
    static const GLenum format = GL_RGBA;
    static const GLenum internal_format = GL_RGBA32I;

    static const GLint components = 4;

    static void set_uniform(GLint location, const ivec4& value)
    {
        glUniform4iv(location, 1, (GLint*)&value);
    }

    static void set_memory_location(const ivec4& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        *((ivec4*)location) = v;
    }
};

template <> struct gltype_info<mat2>
{
    static const GLenum type = GL_FLOAT;

    static const GLint components = 4;

    static void set_uniform(GLint location, const mat2& value)
    {
		glUniformMatrix2fv(location, 1, false, (GLfloat*)&value);
    }

    static void set_float_array(const mat2& in, float* v)
    {

		v[0] = in[0].x; v[1] = in[0].y;
		v[2] = in[1].x; v[3] = in[1].y;
    }

    static mat2 build_from_floats(const float* v)
    {
        return mat2(v[0],v[1],
                    v[2],v[3]);
    }

    static void set_memory_location(const mat2& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        if (!row_major) {
            *((vec2*)(location+0*matrix_stride)) = v[0];
            *((vec2*)(location+1*matrix_stride)) = v[1];
        } else {
            mat2 t = glm::transpose(v);
            *((vec2*)(location+0*matrix_stride)) = t[0];
            *((vec2*)(location+1*matrix_stride)) = t[1];
        }
    }
};

template <> struct gltype_info<mat3>
{
    static const GLenum type = GL_FLOAT;

    static const GLint components = 9;

    static void set_uniform(GLint location, const mat3& value)
    {
		glUniformMatrix3fv(location, 1, false, (GLfloat*)&value);
    }

    static void set_float_array(const mat3& in, float* v)
    {

		v[0] = in[0].x; v[1] = in[0].y; v[2] = in[0].z; 
		v[3] = in[1].x; v[4] = in[1].y; v[5] = in[1].z; 
        v[6] = in[2].x; v[7] = in[2].y; v[8] = in[2].z;

    }

    static mat3 build_from_floats(const float* v)
    {
        return mat3(v[0],v[1],v[2],
                    v[3],v[4],v[5],
                    v[6],v[7],v[8]);
    }

    static void set_memory_location(const mat3& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        if (!row_major) {
            *((vec3*)(location+0*matrix_stride)) = v[0];
            *((vec3*)(location+1*matrix_stride)) = v[1];
            *((vec3*)(location+2*matrix_stride)) = v[2];
        } else {
            mat3 t = glm::transpose(v);
            *((vec3*)(location+0*matrix_stride)) = t[0];
            *((vec3*)(location+1*matrix_stride)) = t[1];
            *((vec3*)(location+2*matrix_stride)) = t[2];
        }
    }
};

template <> struct gltype_info<mat4>
{
    static const GLenum type = GL_FLOAT;

    static const GLint components = 16;

    static void set_uniform(GLint location, const mat4& value)
    {
		glUniformMatrix4fv(location, 1, false, (GLfloat*)&value);
    }

    static void set_float_array(const mat4& in, float* v)
    {

		v[0] = in[0].x; v[1] = in[0].y; v[2] = in[0].z; v[3] = in[0].w;
		v[4] = in[1].x; v[5] = in[1].y; v[6] = in[1].z; v[7] = in[1].w;
        v[8] = in[2].x; v[9] = in[2].y; v[10] = in[2].z; v[11] = in[2].w;
        v[12] = in[3].x; v[13] = in[3].y; v[14] = in[3].z; v[15] = in[3].w;

    }

    static mat4 build_from_floats(const float* v)
    {
        return mat4(v[0],v[1],v[2],v[3],
					v[4],v[5],v[6],v[7],
					v[8],v[9],v[10],v[11],
					v[12],v[13],v[14],v[15]);
    }

    static void set_memory_location(const mat4& v, byte* location, 
                                    size_t matrix_stride, bool row_major)
    {
        if (!row_major) {
            *((vec4*)(location+0*matrix_stride)) = v[0];
            *((vec4*)(location+1*matrix_stride)) = v[1];
            *((vec4*)(location+2*matrix_stride)) = v[2];
            *((vec4*)(location+3*matrix_stride)) = v[3];
        } else {
            mat4 t = glm::transpose(v);
            *((vec4*)(location+0*matrix_stride)) = t[0];
            *((vec4*)(location+1*matrix_stride)) = t[1];
            *((vec4*)(location+2*matrix_stride)) = t[2];
            *((vec4*)(location+3*matrix_stride)) = t[3];
        }
    }
};

#endif
