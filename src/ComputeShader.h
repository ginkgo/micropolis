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
        
        void dispatch(int group_count)     { dispatch(ivec3(group_count, 1,1)); }
        void dispatch(int w, int h)        { dispatch(ivec3(w,h,1));            }
        void dispatch(ivec2 group_count)   { dispatch(ivec3(group_count, 1));   }
        void dispatch(int w, int h, int d) { dispatch(ivec3(w,h,d));            }
        
    };

}
