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


#ifndef UTILITY_H
#define UTILITY_H

#include "common.h"
#include <sstream>
#include <iostream>
#include <iomanip>

#define THOUSAND 1000ULL
#define MILLION  1000000ULL
#define BILLION  1000000000ULL

#define KILO THOUSAND
#define MEGA MILLION
#define GIGA BILLION

#define KIBI_SHIFT 10
#define MEBI_SHIFT 20
#define GIBI_SHIFT 30

#define KIBI (1L << KIBI_SHIFT)
#define MEBI (1L << MEBI_SHIFT)
#define GIBI (1L << GIBI_SHIFT)

uint64_t nanotime();

string with_commas(long n);

string memory_size(size_t size);

/**
 * Test if a file exists on the filesystem.
 * @param filename Name of the file
 * @return True if the file exists.
 */
bool file_exists(const string &filename);


void make_screenshot();

/**
 * Read the content of a text file.
 * @param filename Name of the file.
 * @return String with the whole content of the file.
 */
string read_file(const string &filename);


/**
 * Check for OpenGL errors.
 * Prints a message to STDERR if an error has been found.
 */
void get_errors();


/**
* Attempt to set ARB_debug_output callback.
* This only does something when usign the Debug build.
*/
void set_GL_error_callbacks();



string reverse (const string& s);

vec2 project (const vec4& p);

template <typename T> T minimum (T a, T b)
{
    return a < b ? a : b;
}

template <typename T> T maximum (T a, T b)
{
    return a > b ? a : b;
}

class BBox
{
    public:

    vec3 min;
    vec3 max;

    BBox():
        min(std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity()),
        max(-std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()) {}

    void add_point(const vec3& p) {
        min.x = minimum(p.x, min.x);
        min.y = minimum(p.y, min.y);
        min.z = minimum(p.z, min.z);

        max.x = maximum(p.x, max.x);
        max.y = maximum(p.y, max.y);
        max.z = maximum(p.z, max.z);
    }     

    vec3 size() const {
        return vec3(max.x-min.x, max.y-min.y, max.z-min.z);
    }

    vec3 center() const {
        return (max + min) * 0.5f;
    }

    void clear() {
        min = vec3( std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity());
        max = vec3(-std::numeric_limits<float>::infinity(),
                   -std::numeric_limits<float>::infinity(),
                   -std::numeric_limits<float>::infinity());
    }
};

class Bound
{
public:

    
    vec2 min;
    vec2 max;


    Bound()
        : min( std::numeric_limits<float>::infinity(),
               std::numeric_limits<float>::infinity())
        , max(-std::numeric_limits<float>::infinity(),
              -std::numeric_limits<float>::infinity()) {}

    Bound(float xmin, float ymin, float xmax, float ymax)
        : min(xmin, ymin)
        , max(xmax, ymax) {}

    void add_point(const vec2& p) {
        min.x = minimum(p.x, min.x);
        min.y = minimum(p.y, min.y);

        max.x = maximum(p.x, max.x);
        max.y = maximum(p.y, max.y);
    }     

    vec2 size() {
        return vec2(max.x-min.x, max.y-min.y);
    }
};


class Keyboard
{

    int last_state[GLFW_KEY_LAST+1];
    GLFWwindow* window;
    
public:

    Keyboard(GLFWwindow* window);
    
    void update();

    bool pressed(int key) const;
    bool released(int key) const;
    bool is_down(int key) const;
    bool is_up(int key) const;
    
};


template <typename T>
inline std::ostream& operator<< (std::ostream& os, 
                                 const glm::detail::tvec2<T>& v) {
    return os << v.x << " " << v.y;
}

template <typename T>
inline std::ostream& operator<< (std::ostream& os, 
                                 const glm::detail::tvec3<T>& v) {
    return os << v.x << " " << v.y << " " << v.z;
}

template <typename T>
inline std::ostream& operator<< (std::ostream& os, 
                                 const glm::detail::tvec4<T>& v) {
    return os << v.x << " " << v.y << " " << v.z << " " << v.w;
}


template <typename T>
inline std::istream& operator>> (std::istream& is, 
                                 glm::detail::tvec2<T>& v) {
    return is >> v.x >> v.y;
}


template <typename T>
inline std::istream& operator>> (std::istream& is, 
                                 glm::detail::tvec3<T>& v) {
    return is >> v.x >> v.y >> v.z;
}


template <typename T>
inline std::istream& operator>> (std::istream& is, 
                                 glm::detail::tvec4<T>& v) {
    return is >> v.x >> v.y >> v.z >> v.w;
}



/**
 * Parse a string variable and store inside a statically typed variable.
 * This is the default implementation which should work with types that 
 * implement C++ stream operators.
 * @param s Parsed input string.
 * @param t Output parameter for parsed variable.
 * @return true in case of success, false otherwise.
 */
template<typename T> inline bool from_string(const string& s, T& t)
{
    std::stringstream ss(s);
    
    T tmp;

    if ((ss >> tmp).fail()) {
        return false;
    }
    
    t = tmp;
    return true;
}


template<> inline bool from_string(const string& s, bool& t)
{
    if (s == "true") {
        t = true;
        return true;
    } 
    if (s == "false") {
        t = false;
        return true;
    }

    std::stringstream ss(s);
    
    bool tmp;

    if ((ss >> tmp).fail()) {
        return false;
    }
    
    t = tmp;
    return true;
}

/**
 * Convert an arbitrary type variable to a string.
 * This is the default implementation which should work with types that 
 * implement C++ stream operators.
 * @param t Input value.
 * @return The string representing the value.
 */
template<typename T> inline string to_string(const T& t)
{
    std::stringstream ss;
    
    ss << std::boolalpha;
    ss << t;

    return ss.str();    
}

/**
 * Parse a string variable and store inside a statically typed variable.
 * This is a trivial implementation for strings which simply creates a copy.
 */
template<> inline bool from_string(const string& s, string& t)
{
    t = s;
    
    return true;
}

/**
 * Convert an arbitrary type variable to a string.
 * This is a trivial implementation for strings which simply creates a copy.
 */
template<> inline string to_string(const string& t)
{
    return t;
}

template <typename T>
inline T square (T a) 
{
    return a*a;
}


template <typename T>
void debug_print(const T& v)
{
    cout << v << endl;
}

template <typename T, typename ... Rest>
void debug_print(const T& v, Rest&& ... rest)
{
    cout << v << "   --   ";
    debug_print(rest...);
}

template <typename I>
inline I round_up_by(I i, I n)
{
    return (i % n == 0) ? i : (i/n+1)*n;
}

template <typename I>
inline I round_up_div(I n, I d)
{
    return n/d + ((n%d == 0) ? 0 : 1);
}


#endif
