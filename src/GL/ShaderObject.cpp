#include "ShaderObject.h"

#include "Config.h"
#include "glutils.h"

#include <fstream>
#include <boost/regex.hpp>
#include <boost/format.hpp>

#define LOG_BUFFER_SIZE 1024*64

GL::ShaderObject::ShaderObject(const string& shader, const string& material, GLenum shader_type)
    : shadername(shader)
{
    shader_handle = compile_shader_object(shader, material, shader_type);
}


GL::ShaderObject::~ShaderObject()
{
    if (shader_handle != 0) {
        glDeleteShader(shader_handle);
    }

    shader_handle = 0;
}


bool GL::ShaderObject::valid() const
{
    return shader_handle != 0;
}


bool GL::ShaderObject::invalid() const
{
    return shader_handle == 0;
}


void GL::ShaderObject::attach_to(GLuint program_handle) const
{
    glAttachShader(program_handle, shader_handle);
}


bool GL::ShaderObject::load_shader_source(const string& shader, 
                                          const string& material,
                                          const string& file_extension)
{
    string shaderfile   = config.shader_dir() +"/"+shader  +file_extension;

    return read_file(shaderfile);
}

bool GL::ShaderObject::read_file(const string& filename)
{
    const boost::regex include_pattern("@include\\s+<(.+)>.*");
    boost::match_results<std::string::const_iterator> match;
    
    if (file_to_index_map.count(filename) > 0) return true;

    int fileno = file_to_index_map.size();

    file_to_index_map[filename] = fileno;
    index_to_file_map[fileno] = filename;

    ss << "#line 1 " << fileno << endl;

    std::ifstream is(filename.c_str());

    if (!is) {
        return false;
    }
    
    std::string line;
        
    int lineno = 0;
    while (std::getline(is,line)) {
        lineno++;

        if (boost::regex_match(line, match, include_pattern)) {
            string includefile = config.shader_dir()+"/"+match.str(1);

            if (!read_file(includefile)) {
                return false;
            }

            ss << "#line " << lineno+1 << " " << fileno << endl;
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
GLuint GL::ShaderObject::compile_shader_object(const string& shader, 
                                               const string& material,
                                               GLenum type) 
{
    string file_extension;

    switch(type) {
    case GL_VERTEX_SHADER: file_extension = ".vert"; break;
    case GL_GEOMETRY_SHADER: file_extension = ".geom"; break;
    case GL_FRAGMENT_SHADER: file_extension = ".frag"; break;
#ifdef GL_VERSION_4_0
    case GL_TESS_CONTROL_SHADER: file_extension = ".tess_ctrl"; break;
    case GL_TESS_EVALUATION_SHADER: file_extension = ".tess_eval"; break;
#endif
#ifdef GL_VERSION_4_3
    case GL_COMPUTE_SHADER: file_extension = ".compute"; break;
#endif
    default:
        assert(0);
    }

    if (!load_shader_source(shader, material, file_extension)) {
        return 0;
    }

    if (config.verbosity_level() >= 2) {
        cout << boost::format("\t %1%/%2%%3%") % config.shader_dir() % shader % file_extension << endl;
    }
    
    string source = ss.str();

    shader_handle = glCreateShader(type);

    const char * csource = source.c_str();
    GLint source_length = source.size();

    
    glShaderSource(shader_handle, 1, &csource, &source_length);

    glCompileShader(shader_handle);

    GLint status;

    glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &status);
    
    if (config.verbosity_level() > 0 || status == GL_FALSE) {
        print_shader_log();
    }

    if (status == GL_FALSE) {
        glDeleteShader(shader_handle);
        return 0;
    }

    return shader_handle;
}


void GL::ShaderObject::print_shader_log()
{
    const boost::regex pattern("(.+)([0-9]+):([0-9]+)(.+)");
    boost::match_results<std::string::const_iterator> match;
    
    char logBuffer[LOG_BUFFER_SIZE];
    GLsizei length;
  
    logBuffer[0] = '\0';
    glGetShaderInfoLog(shader_handle, LOG_BUFFER_SIZE, &length,logBuffer);

    if (length == 0) return;
    
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << shadername << " shader build log:" << endl;

    string line;
    
    std::stringstream ss(logBuffer);

    while(std::getline(ss, line)) {
        if (boost::regex_match(line, match, pattern)) {
            cout << match.str(1) << index_to_file_map[stoi(match.str(2))] << ":" << match.str(3) << match.str(4) << endl;
        } else {
            cout << line << endl;
        }
    }
}
