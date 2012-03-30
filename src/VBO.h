#ifndef VBO_H

#include "common.h"

namespace GL
{
	class Shader;

	/**
	 * Represents a (mirrored) OpenGL vertex buffer object.
	 */
	class VBO
	{
		GLuint _buffer;
		GLuint _size;
		mutable map<GLuint, GLuint> _vaos;

		vector<vec3> _vertices;
		size_t _vertex_count;

	public:

		VBO(size_t vertex_cnt);
		virtual ~VBO();

		void clear();
		void vertex(const vec2& v);
		void vertex(const vec3& v);
		void vertex(float x, float y);
		void vertex(float x, float y, float z);
		void send_data(bool stream = true);

		void draw(GLenum mode, const Shader& shader) const;

	private:

		void create_vao(const Shader& shader) const;
	};

}

#endif
