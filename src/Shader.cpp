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


#include "Shader.h"

#include "glutils.h"
#include "ShaderObject.h"

#include "Config.h"


GL::Shader::Shader(const string& shader,
                   const string& material) :
    _valid(false),
    _program(0),
    _name(shader)
{

    // Load an compile shaders
    ShaderObject vertex_shader(shader, material, GL_VERTEX_SHADER);
    ShaderObject fragment_shader(shader, material, GL_FRAGMENT_SHADER);
    ShaderObject geometry_shader(shader, material, GL_GEOMETRY_SHADER);
        
    // Test for errors
    if (vertex_shader.invalid() || fragment_shader.invalid()) {
        return;
    }

    _program = glCreateProgram();

    // Attach shaders to program
    vertex_shader.attach_to(_program);
    fragment_shader.attach_to(_program);

    if (geometry_shader.valid()) {
        geometry_shader.attach_to(_program);
    }


#ifdef GL_VERSION_4_0
    ShaderObject tess_ctrl_shader(shader, material, GL_TESS_CONTROL_SHADER);
    ShaderObject tess_eval_shader(shader, material, GL_TESS_EVALUATION_SHADER);

    if (tess_eval_shader.valid() && tess_ctrl_shader.valid()) {
        tess_ctrl_shader.attach_to(_program);
        tess_eval_shader.attach_to(_program);
    }
#endif

    link();
}

GL::Shader::~Shader()
{
    if (_program != 0) {
        glDeleteProgram(_program);
    }

    _program = 0;
}

void GL::Shader::link()
{
    // Link and check for success
    glLinkProgram(_program);

    GLint status;

    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    
    if (status == GL_FALSE || config.verbose()) {
        print_program_log(_program, _name);
    }

    if (status == GL_FALSE) {
        if (_program != 0) {
            glDeleteProgram(_program);
        }
        _program = 0;
        
        return;
    }

    _valid = true;

    // Query active uniforms    
    GLint active_uniforms;
    
    glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &active_uniforms);

    char buffer[1000];

    for (GLint uniform = 0; uniform < active_uniforms; ++uniform) {
        glGetActiveUniformName(_program, uniform, 1000, NULL, buffer);
        string uniform_name(buffer);
        _uniform_map[uniform_name] = glGetUniformLocation(_program, buffer);
    }
}

GLint GL::Shader::get_attrib_count() const
{
    GLint attrib_count;
    glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &attrib_count);

    return attrib_count;
}

string GL::Shader::get_attrib_name(GLint index) const
{
    assert(index < get_attrib_count());

    GLchar buffer[1000];
    GLint attrib_size;
    GLenum attrib_type;

    glGetActiveAttrib(_program, index, 1000, NULL, &attrib_size, &attrib_type, buffer);

    return string(buffer);
}

