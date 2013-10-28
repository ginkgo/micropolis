#include "ShaderObject.h"

#include "Config.h"
#include "glutils.h"

#include <fstream>
#include <boost/regex.hpp>



GL::ShaderObject::ShaderObject(const string& shader, const string& material, GLenum shader_type)
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
GLuint GL::ShaderObject::compile_shader_object(const string& shader, 
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
#ifdef GL_VERSION_4_3
    case GL_COMPUTE_SHADER: file_extension = ".compute"; break;
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
        print_shader_log(shader_handle, config.shader_dir() +"/"+shader+file_extension);
    }

    if (status == GL_FALSE) {
        glDeleteShader(shader_handle);
        return 0;
    }

    return shader_handle;
}
