#pragma once

#include "common.h"


namespace GL
{
    
    void insert_filename_and_print(char* clog, const string& filename);
    void print_shader_log(GLuint shader, const string& filename);
    void print_program_log(GLuint program, const string& filename);

}
