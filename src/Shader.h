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
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


/** @file Shader.h */

#ifndef SHADER_H
#define SHADER_H

#include "common.h"
#include "type_info.h"
#include "Texture.h"

namespace GL
{
	/**
	 * Represents a compiled GLSL shader.
	 */
	class Shader
	{
		bool _valid; /**< State variable for testing compilation success */

		GLuint _program; /**< Shader program handle. */

		GLuint _vertex_shader; /**< Vertex shader handle. */
		GLuint _geometry_shader; /**< Geometry shader handle */
		GLuint _fragment_shader; /**< Fragment shader handle */
		GLuint _tess_ctrl_shader; /**< Tessellation control shader handle */
		GLuint _tess_eval_shader; /**< Tessellation evaluation shader handle */

		string _name;

		typedef boost::unordered_map<string, GLint> UniformMap;
		UniformMap _uniform_map;

		typedef boost::unordered_map<string, GLuint> UniformBlockMap;
		UniformBlockMap _uniform_block_map;

    public:
    
		/**
		 * Load and compile a shader.
		 * @param shader Name of the shader. Don't add a file-extension or dir.
		 * @param material Name of the material. Optional.
		 */
		Shader(const string& shader, const string& material="");
		~Shader();

		/**
		 * Bind the shader object
		 */
		void bind() const
		{
			glUseProgram(_program);
		}

		/**
		 * Unbind the shader object
		 */
		void unbind() const
		{
			glUseProgram(0);
		}

		/**
		 * Set a uniform.
		 * @param uniform_name Name of uniform.
		 * @param value The value to set the uniform to.
		 */
		template<typename T>
		void set_uniform(const string& uniform_name, const T& value)
		{
			UniformMap::iterator it = _uniform_map.find(uniform_name);
        
			if (it == _uniform_map.end())
				return;

			gltype_info<T>::set_uniform(it->second, value);
		}

		void set_uniform(const string& uniform_name, const Tex& texture)
		{
			UniformMap::iterator it = _uniform_map.find(uniform_name);
        
			if (it == _uniform_map.end())
				return;

			gltype_info<GLint>::set_uniform(it->second, texture.get_unit_number());
		}

		void set_uniform(const string& uniform_name, const TextureBuffer& texture)
		{
			set_uniform(uniform_name, (Tex&) texture);
		}

		bool has_uniform(const string& uniform_name) const {
			return (_uniform_map.count(uniform_name) > 0);
		}

		// void set_uniform(const string& uniform_name, const TextureArray& texture)
		// {
		//     UniformMap::iterator it = _uniform_map.find(uniform_name);
        
		//     if (it == _uniform_map.end())
		//         return;

		//     gltype_info<GLint>::set_uniform(it->second, texture.get_unit_number());
		// }

		// void set_uniform_block(const string& block_name, 
		//                        const UniformBuffer& UBO) 
		// {
		//     UniformBlockMap::iterator it = _uniform_block_map.find(block_name);
        
		//     if (it == _uniform_block_map.end())
		//         return;

		//     glUniformBlockBinding(_program, it->second, UBO.get_binding());
		// };

		/**
		 * Return the location of an attrib.
		 */
		GLint get_attrib_location(const string& attrib_name) const
		{
			return glGetAttribLocation(_program, attrib_name.c_str());
		}

		/**
		 * Return the shader program handle.
		 */
		GLuint get_program_ID() const
		{
			return _program;
		}

		GLint get_attrib_count() const;

		string get_attrib_name(GLint index) const;

		/**
		 * Used for checking if a shader has compiled correctly.
		 */
		bool is_valid() const
		{
			return _valid;
		}
    
    private:

		void clean_up();
	};

}
#endif
