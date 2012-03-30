#include "VBO.h"

#include "Shader.h"


using namespace GL;

GL::VBO::VBO(size_t vertex_cnt):
	_buffer(0),
	_size(vertex_cnt * sizeof(vec3)),
	_vertices(vertex_cnt),
	_vertex_count(0)
{
	glGenBuffers(1, &_buffer);

	_vertices.resize(vertex_cnt);
	
	send_data();
}


GL::VBO::~VBO()
{
	glDeleteBuffers(1, &_buffer);

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
	_vertices.at(_vertex_count) = vec3(v.x,v.y,0);
	_vertex_count++;
}


void GL::VBO::vertex(const vec3& v)
{
	_vertices.at(_vertex_count) = v;
	_vertex_count++;
}

void GL::VBO::vertex(float x, float y)
{
	vertex(vec3(x,y,0));
}

void GL::VBO::vertex(float x, float y, float z)
{
	vertex(vec3(x,y,z));
}


void GL::VBO::send_data(bool stream)
{
	glBindBuffer(GL_ARRAY_BUFFER, _buffer);

	glBufferData(GL_ARRAY_BUFFER, _size, _vertices.data(), 
				 stream?GL_STREAM_DRAW:GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	glBindBuffer(GL_ARRAY_BUFFER, _buffer);
	
	GLint location = shader.get_attrib_location("vertex");

	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	_vaos[shader.get_program_ID()] = vao;
}
