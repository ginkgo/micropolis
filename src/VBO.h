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


#ifndef VBO_H
#define VBO_H

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

        bool full() const;
        bool empty() const;

	private:

		void create_vao(const Shader& shader) const;
	};

}

#endif
