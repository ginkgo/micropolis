#pragma once

#include "common.h"

namespace GL
{

    class ShaderObject
    {
        GLuint shader_handle;
        string shadername;
        
        std::stringstream ss;
        map<string, int> file_to_index_map;
        map<int, string> index_to_file_map;
        
    public:

        ShaderObject(const string& shader, const string& material, GLenum shader_type);
        ~ShaderObject();

        bool valid() const;
        bool invalid() const;
        void attach_to(GLuint program_handle) const;

    private:

        GLuint compile_shader_object(const string& shader,
                                     const string& material,
                                     GLenum type);
        bool load_shader_source(const string& shader,
                                const string& material,
                                const string& file_extension);

        bool read_file(const string& filename);

        void print_shader_log();

            
    };


}
