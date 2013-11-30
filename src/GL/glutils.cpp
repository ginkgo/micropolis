
#include "glutils.h"


#include <boost/regex.hpp>

#define LOG_BUFFER_SIZE 1024*64

void GL::insert_filename_and_print(char* clog, const string& filename)
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
void GL::print_program_log(GLuint program, const string& filename)
{
    char logBuffer[LOG_BUFFER_SIZE];
    GLsizei length;
  
    logBuffer[0] = '\0';
    glGetProgramInfoLog(program, LOG_BUFFER_SIZE, &length,logBuffer);
  
    if (length > 0) {
        cout << "--------------------------------------------------------------------------------" << endl;
        cout << filename << " program link log:" << endl;
        insert_filename_and_print(logBuffer, filename);
    }
};


/**
 * Print a shader error log to shell.
 * @param program OpenGL Shader handle.
 * @param filename Filename of the compiled file to give feedback.
 */
void GL::print_shader_log(GLuint shader, const string& filename)
{
    char logBuffer[LOG_BUFFER_SIZE];
    GLsizei length;
  
    logBuffer[0] = '\0';
    glGetShaderInfoLog(shader, LOG_BUFFER_SIZE, &length,logBuffer);

    if (length > 0) {
        cout << "--------------------------------------------------------------------------------" << endl;
        cout << filename << " shader build log:" << endl;
        insert_filename_and_print(logBuffer, filename);
    }
};
