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


#include "utility.h"

#include <fstream>

#include <time.h>

uint64_t nanotime()
{
#ifdef linux
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (unsigned long long) tp.tv_sec * BILLION + (unsigned long long) tp.tv_nsec;
#else
	LARGE_INTEGER current;
	LARGE_INTEGER m_ticksPerSec;
	QueryPerformanceFrequency(&m_ticksPerSec);
	QueryPerformanceCounter(&current);
	return (unsigned long long)((double)current.QuadPart / m_ticksPerSec.QuadPart * BILLION);
#endif
}

string with_commas(long n)
{
    std::stringstream ss;

    int i = 0;
    do {
        
        int d = n % 10;
        n = n / 10;

        ss << d;

        ++i;
        if (n > 0 && i == 3) {
            i = 0;
            ss << ',';
        }
    } while (n > 0);

    return reverse(ss.str());
}

string memory_size(size_t size)
{
    const vector<string> units = {"B ", "KiB ", "MiB ", "GiB ", "Tib ", "PiB ", "EiB ", "ZiB "};
    vector<size_t> values;

    if (size == 0) return "0B ";
    
    while (size > 0) {
        values.push_back(size % 1024);
        size = size / 1024;
    }
    
    std::stringstream ss;

    // for (int i = values.size()-1; i >= 0; --i) {
    //     if (values[i] > 0) {
    //         ss << values[i] << units[i];
    //     }
    // }

    size_t s = values.size();
    
    ss << values[s-1]
       << units[s-1];

    return ss.str();
}

string reverse (const string& s)
{
    return string(s.rbegin(), s.rend());
}

bool file_exists(const string &filename)
{
    std::ifstream ifile(filename.c_str());
    return ifile.good();
}

string read_file(const string &filename)
{
    std::ifstream ifile(filename.c_str());

    return string(std::istreambuf_iterator<char>(ifile),
        std::istreambuf_iterator<char>());
}


void get_errors()
{
    static int call_count = 0;

    // We don't need get_errors if we use ARB_debug_output
    if (true) {
        GLenum error = glGetError();

        if (error != GL_NO_ERROR) {
            if (call_count == 16) return;

            switch (error) {
            case GL_INVALID_ENUM:
                cerr << "GL: enum argument out of range." << endl;
                break;
            case GL_INVALID_VALUE:
                cerr << "GL: Numeric argument out of range." << endl;
                break;
            case GL_INVALID_OPERATION:
                cerr << "GL: Operation illegal in current state." << endl;
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                cerr << "GL: Framebuffer object is not complete." << endl;
                break;
            case GL_OUT_OF_MEMORY:
                cerr << "GL: Not enough memory left to execute command." << endl;
                break;
            default:
                cerr << "GL: Unknown error." << endl;
            }

            ++call_count;
        }
    }
}




/**
 * Callback method for ARB_debug_output.
 * @param source Source of the message.
 * @param type Type of the message.
 * @param id Identification number of the message.
 * @param severity Severity of the message.
 * @param length Length of the messge in bytes(?).
 * @param message Message.
 * @param userParam NULL.
 */
GLvoid APIENTRY opengl_debug_callback(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      GLvoid* userParam)
{
    // A small hack to avoid getting the same error message again and again.
    static int call_count = 0;

    string source_name = "Unknown";
    string type_name = "Unknown";
    string severity_name = "Unknown";

    // Find source name
    switch (source) {
    case GL_DEBUG_SOURCE_API_ARB:
        source_name = "OpenGL"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        source_name = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        return; // The shader compiler log gives better feedback
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        source_name = "External debuggers/middleware"; break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        source_name = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        source_name = "Other"; break;
    }

    // Find type name
    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
        type_name = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        type_name = "Deprecated Behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        type_name = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        type_name = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        type_name = "Performance"; break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        type_name = "Other"; break;
    }

    // Find severity name.
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        severity_name = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        severity_name = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        severity_name = "low"; break;
    }

    if (call_count < 8) {
        // Print all received message information.
        cerr << "Caught OpenGL debug message:" << endl;
        cerr << "\tSource: " << source_name << endl;
        cerr << "\tType: " << type_name << endl;
        cerr << "\tSeverity: " << severity_name << endl;
        cerr << "\tID: " << id << endl;
        cerr << "\tMessage: " << message << endl;
        cerr << endl;
    } else if (severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
        exit(1);
    }
    
    ++call_count;
    
}


void set_GL_error_callbacks()
{
    if (FLEXT_ARB_debug_output) {
        // Enable synchronous callbacks.
        // Otherwise we might not get usable stack-traces.
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    
        // Enable ALL debug messages.
        glDebugMessageControlARB(GL_DONT_CARE,
                                 GL_DONT_CARE,
                                 GL_DONT_CARE,
                                 0, NULL,
                                 GL_TRUE);

        // Set callback function
        glDebugMessageCallbackARB(opengl_debug_callback, NULL);
    }
}





vec2 project (const vec4& p)
{
    return vec2(p.x, p.y) / p.w;
}



Keyboard::Keyboard(GLFWwindow* window)
    : window(window)
{
    update();
}


void Keyboard::update()
{
    for (int i = 0; i <= GLFW_KEY_LAST; ++i) {
        last_state[i] = glfwGetKey(window, i);
    }
}




bool Keyboard::pressed(int key) const
{
    return last_state[key] == GL_FALSE && glfwGetKey(window, key) == GL_TRUE;
}


bool Keyboard::released(int key) const
{
    return last_state[key] == GL_TRUE && glfwGetKey(window, key) == GL_FALSE;
}


bool Keyboard::is_down(int key) const
{
    return glfwGetKey(window,key) == GL_TRUE;
}


bool Keyboard::is_up(int key) const
{
    return glfwGetKey(window,key) == GL_FALSE;
}
