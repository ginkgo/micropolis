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


#include "Shader.h"

#include <boost/regex.hpp>
#include <fstream>
#include "Config.h"

#define LOG_BUFFER_SIZE 1024*64

// Anonymous namespace
namespace {

    void insert_filename_n_print(char* clog, const string& filename)
    {
        string line;

        const boost::regex pattern("(.+)([0-9]+):([0-9]+)(.+)");
        const boost::regex pattern2("([0-9]+)\\(([0-9]+)\\) :(.+)");
        boost::match_results<std::string::const_iterator> match;
        
        std::stringstream ss(clog);

        while(std::getline(ss, line)) {
            if (boost::regex_match(line, match, pattern)) {
                cout << filename << ":" << match.str(3) << ":" << match.str(2) << "" << match.str(4) << endl;
            } else if (boost::regex_match(line, match, pattern2)) {
                cout << filename << ":" << match.str(2) << ":" << match.str(1) << ":" << match.str(3) << endl;
            } else {
                cout << line << endl;
            }
        }
    }

    /**
     * Print a shader program error log to the shell.
     * @param program OpenGL Shader Program handle.
     * @param filename Filename of the linked file(s) to give feedback.
     */
    void program_log(GLuint program, const string& filename)
    {
        char logBuffer[LOG_BUFFER_SIZE];
        GLsizei length;
  
        logBuffer[0] = '\0';
        glGetProgramInfoLog(program, LOG_BUFFER_SIZE, &length,logBuffer);
  
        if (length > 0) {
            cout << "--------------------------------------------------------------------------------" << endl;
            cout << filename << " program link log:" << endl;
            insert_filename_n_print(logBuffer, filename);
            //cout << logBuffer << endl;
        }
    };


    /**
     * Print a shader error log to shell.
     * @param program OpenGL Shader handle.
     * @param filename Filename of the compiled file to give feedback.
     */
    void shader_log(GLuint shader, const string& filename)
    {
        char logBuffer[LOG_BUFFER_SIZE];
        GLsizei length;
  
        logBuffer[0] = '\0';
        glGetShaderInfoLog(shader, LOG_BUFFER_SIZE, &length,logBuffer);

        if (length > 0) {
            cout << "--------------------------------------------------------------------------------" << endl;
            cout << filename << " shader build log:" << endl;
            insert_filename_n_print(logBuffer, filename);
            // cout << logBuffer << endl;
        }
    };

    bool load_shader_source(const string& shader, 
                            const string& material,
                            const string& file_extension,
                            std::stringstream& ss)
    {
        const int MAX_LENGTH = 1024;
        char buffer[MAX_LENGTH];

        const boost::regex include_pattern("@include\\s+<(.+)>.*");
        boost::match_results<std::string::const_iterator> match;


        string shaderfile   = config.shader_dir() +"/"+shader  +file_extension;

        std::ifstream is(shaderfile.c_str());

        if (!is) {
            return false;
        }

        while (is.getline(buffer, MAX_LENGTH)) {
            std::string line(buffer);
        
            if (boost::regex_match(line, match, include_pattern)) {
                //Note: using match.str(1) instead of match[1] is a workaround
                //for VS2010
                string includefile = config.shader_dir()+"/"+match.str(1);

                if (!file_exists(includefile)) {
                    cerr << "Could not find file " << includefile << endl;
                    return false;
                }
            
                ss << read_file(includefile);
            } else {
                ss << line << endl;
            }
        }

        return true;
    }

    /**
     * Load and compile a GLSL shader file.
     * @param filename GLSL file name.
     * @param type GLSL shader type enum.
     * @return Shader handle in case of success, 0 otherwise.
     */
    GLuint compile_shader_object(const string& shader, 
                                 const string& material,
                                 GLenum type) 
    {
        std::stringstream ss;

        string file_extension;

        switch(type) {
        case GL_VERTEX_SHADER: file_extension = ".vert"; break;
        case GL_GEOMETRY_SHADER: file_extension = ".geom"; break;
        case GL_FRAGMENT_SHADER: file_extension = ".frag"; break;
#ifdef GL_VERSION_4_0
        case GL_TESS_CONTROL_SHADER: file_extension = ".tess_ctrl"; break;
        case GL_TESS_EVALUATION_SHADER: file_extension = ".tess_eval"; break;
#endif
        default:
            assert(0);
        }

        if (!load_shader_source(shader, material, file_extension, ss)) {
            return 0;
        }

        string source = ss.str();

        GLuint shader_handle = glCreateShader(type);

        const char * csource = source.c_str();
        GLint source_length = source.size();

        glShaderSource(shader_handle, 1, &csource, &source_length);

        glCompileShader(shader_handle);

        GLint status;

        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &status);
    
        if (config.verbose() || status == GL_FALSE) {
            shader_log(shader_handle, config.shader_dir() +"/"+shader+file_extension);
        }

        if (status == GL_FALSE) {
            glDeleteShader(shader_handle);
            return 0;
        }

        return shader_handle;
    }
}

namespace GL
{

    Shader::Shader(const string& shader,
                   const string& material) :
        _valid(false),
        _program(0),
        _vertex_shader(0),
        _geometry_shader(0),
        _fragment_shader(0),
        _tess_ctrl_shader(0),
        _tess_eval_shader(0),
        _name(shader)
    {

        // Load an compile shaders
        _vertex_shader = compile_shader_object(shader, material, GL_VERTEX_SHADER);
        _fragment_shader = compile_shader_object(shader, material, GL_FRAGMENT_SHADER);
        _geometry_shader = compile_shader_object(shader, material, GL_GEOMETRY_SHADER);


        // Test for errors
        if (_vertex_shader == 0 || _fragment_shader == 0) {
            clean_up();
            return;
        }

        _program = glCreateProgram();

        // Attach shaders to program
        glAttachShader(_program, _vertex_shader);
        glAttachShader(_program, _fragment_shader);

        if (_geometry_shader != 0) {
            glAttachShader(_program, _geometry_shader);
        }


#ifdef GL_VERSION_4_0
        _tess_ctrl_shader = compile_shader_object(shader, material,
                                                  GL_TESS_CONTROL_SHADER);
        _tess_eval_shader = compile_shader_object(shader, material,
                                                  GL_TESS_EVALUATION_SHADER);

        if (_tess_eval_shader != 0 && _tess_ctrl_shader != 0) {
            glAttachShader(_program, _tess_ctrl_shader);
            glAttachShader(_program, _tess_eval_shader);
        }
#endif

        // Link and check for success
        glLinkProgram(_program);

        GLint status;

        glGetProgramiv(_program, GL_LINK_STATUS, &status);
    
        if (status == GL_FALSE || config.verbose()) {
            program_log(_program, shader);
        }

        if (status == GL_FALSE) {
            clean_up();
            return;
        }

        _valid = true;

        GLint active_uniforms, active_uniform_blocks;

        glGetProgramiv(_program, GL_ACTIVE_UNIFORM_BLOCKS, &active_uniform_blocks);
        glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &active_uniforms);

        char buffer[1000];

        for (GLint uniform = 0; uniform < active_uniforms; ++uniform) {
            glGetActiveUniformName(_program, uniform, 1000, NULL, buffer);
            string uniform_name(buffer);
            _uniform_map[uniform_name] = glGetUniformLocation(_program, buffer);
        }

        for (GLuint uniform_block = 0; 
             uniform_block < GLuint(active_uniform_blocks); ++uniform_block) {
            glGetActiveUniformBlockName(_program, uniform_block, 
                                        1000, NULL, buffer);
            string uniform_block_name(buffer);
            _uniform_block_map[uniform_block_name] = uniform_block;
        }
    }

    Shader::~Shader()
    {
        clean_up();
    }

    void Shader::clean_up()
    {
        if (_program != 0) {
            glDeleteProgram(_program);
        }

        if (_vertex_shader != 0) {
            glDeleteShader(_vertex_shader);
        }

        if (_geometry_shader != 0) {
            glDeleteShader(_geometry_shader);
        }

        if (_fragment_shader != 0) {
            glDeleteShader(_fragment_shader);
        }

        if (_tess_ctrl_shader != 0) {
            glDeleteShader(_tess_ctrl_shader);
        }

        if (_tess_eval_shader != 0) {
            glDeleteShader(_tess_eval_shader);
        }

        _program = _vertex_shader = _geometry_shader = _fragment_shader = 0;
    }

    GLint Shader::get_attrib_count() const
    {
        GLint attrib_count;
        glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &attrib_count);

        return attrib_count;
    }

    string Shader::get_attrib_name(GLint index) const
    {
        assert(index < get_attrib_count());

        GLchar buffer[1000];
        GLint attrib_size;
        GLenum attrib_type;

        glGetActiveAttrib(_program, index, 1000, NULL, &attrib_size, &attrib_type, buffer);

        return string(buffer);
    }

}
