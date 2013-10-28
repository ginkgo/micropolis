#pragma once

#include "Shader.h"


namespace GL
{

    class ComputeShader : public Shader
    {

    public:

        ComputeShader(const string& shader_name);
        ~ComputeShader();

        void dispatch(ivec3 group_count);
        
    };

}
