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


#include "VBO.h"

#include "Shader.h"
#include "Statistics.h"

using namespace GL;



/*----------------------------------------------------------------------------*/
// VBO implementation



GL::VBO::VBO(size_t vertex_cnt)
    : _buffer(vertex_cnt * sizeof(vec4))
	, _vertices(vertex_cnt)
    , _vertex_count(0)
{
}


GL::VBO::~VBO()
{
	for (map<GLuint, GLuint>::iterator i = _vaos.begin(); i != _vaos.end(); ++i) {
		GLuint vao = i->second;

		glDeleteVertexArrays(1, &vao);
	}
}


void GL::VBO::clear()
{
	_vertex_count = 0;
}


void GL::VBO::vertex(const vec2& v)
{
	_vertices.at(_vertex_count) = vec4(v.x,v.y,0,1);
	_vertex_count++;
}


void GL::VBO::vertex(const vec3& v)
{
	_vertices.at(_vertex_count) = vec4(v.x,v.y,v.z,1);
	_vertex_count++;
}


void GL::VBO::vertex(const vec4& v)
{
	_vertices.at(_vertex_count) = v;
	_vertex_count++;
}

void GL::VBO::vertex(float x, float y)
{
	vertex(vec4(x,y,0,1));
}

void GL::VBO::vertex(float x, float y, float z)
{
	vertex(vec4(x,y,z,1));
}


void GL::VBO::send_data(bool stream)
{
    _buffer.bind(GL_ARRAY_BUFFER);
    _buffer.send_subdata(_vertices.data(), 0, _vertices.size() * sizeof(vec4));
    _buffer.unbind();
}


void GL::VBO::draw(GLenum mode, const Shader& shader) const
{
	GLuint program_id = shader.get_program_ID();
	
	if (_vaos.count(program_id) < 1) {
		create_vao(shader);
	}

	GLuint vao = _vaos.at(program_id);

	glBindVertexArray(vao);

	glDrawArrays(mode, 0, _vertex_count);

	glBindVertexArray(0);
}


void GL::VBO::create_vao(const Shader& shader) const
{
	GLuint vao;

	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
    
	_buffer.bind(GL_ARRAY_BUFFER);
	
	GLint location = shader.get_attrib_location("vertex");

	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, 0, NULL);

	_buffer.unbind();
	glBindVertexArray(0);

	_vaos[shader.get_program_ID()] = vao;
}


bool GL::VBO::full() const
{
    return _vertices.size() <= _vertex_count;
}


bool GL::VBO::empty() const
{
    return _vertex_count == 0;
}



/*----------------------------------------------------------------------------*/
// IndirectVBO implementation



GL::IndirectVBO::IndirectVBO(size_t max_vertex_count)
    : _vbuffer(max_vertex_count * sizeof(vec4))
    , _ibuffer(4 * sizeof(GLuint))
    , _max_vertex_count(max_vertex_count)
{
}


GL::IndirectVBO::~IndirectVBO()
{
	for (map<GLuint, GLuint>::iterator i = _vaos.begin(); i != _vaos.end(); ++i) {
		GLuint vao = i->second;

		glDeleteVertexArrays(1, &vao);
	}
}


void GL::IndirectVBO::load_vertices(const vector<vec4>& vertices)
{
    _vbuffer.bind(GL_ARRAY_BUFFER);
    _vbuffer.send_subdata((void*)vertices.data(), 0, vertices.size() * sizeof(vec4));
    _vbuffer.unbind();
}


void GL::IndirectVBO::load_indirection(GLuint count, GLuint instance_count, GLuint first, GLuint base_instance)
{
    struct
    {
        GLuint count;
        GLuint instance_count;
        GLuint first;
        GLuint base_instance;
    } indirection = {count, instance_count, first, base_instance};

    _ibuffer.bind(GL_DRAW_INDIRECT_BUFFER);
    _ibuffer.send_subdata(&indirection, 0, sizeof(indirection));
    _ibuffer.unbind();
}


GLuint GL::IndirectVBO::get_max_vertex_count() const
{
    return _max_vertex_count;
}


GL::Buffer& GL::IndirectVBO::get_vertex_buffer()
{
    return _vbuffer;
}


GL::Buffer& GL::IndirectVBO::get_indirection_buffer()
{
    return _ibuffer;
}


void GL::IndirectVBO::draw(GLenum mode, const Shader& shader) const
{
	GLuint program_id = shader.get_program_ID();
	
	if (_vaos.count(program_id) < 1) {
		create_vao(shader);
	}

	GLuint vao = _vaos.at(program_id);

	glBindVertexArray(vao);

    _ibuffer.bind(GL_DRAW_INDIRECT_BUFFER);
	glDrawArraysIndirect(mode, 0);
    _ibuffer.unbind();

	glBindVertexArray(0);
}



void GL::IndirectVBO::create_vao(const Shader& shader) const
{
	GLuint vao;

	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
    _vbuffer.bind(GL_ARRAY_BUFFER);
	
	GLint location = shader.get_attrib_location("vertex");

	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, 0, NULL);


    _vbuffer.unbind();
	glBindVertexArray(0);


	_vaos[shader.get_program_ID()] = vao;
}
