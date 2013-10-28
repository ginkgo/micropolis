#include "ComputeShader.h"

#include "ShaderObject.h"

GL::ComputeShader::ComputeShader(const string& shader_name)
    : Shader()
{
    _name = shader_name;
    
    ShaderObject compute_shader(shader_name, "", GL_COMPUTE_SHADER);

    if (compute_shader.invalid()) {
        return;
    }
    
    _program = glCreateProgram();
    compute_shader.attach_to(_program);

    link();    
}



GL::ComputeShader::~ComputeShader()
{

}


void GL::ComputeShader::dispatch(ivec3 group_count)
{
    glDispatchCompute((GLuint)group_count.x,
                      (GLuint)group_count.y,
                      (GLuint)group_count.z);
}
