#ifndef UTILITY_H
#define UTILITY_H

#include "common.h"
#include <sstream>
#include <iostream>


void calc_fps(float& fps, float& mspf);

/**
 * Test if a file exists on the filesystem.
 * @param filename Name of the file
 * @return True if the file exists.
 */
bool file_exists(const string &filename);

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
void get_errors(void);

template <typename T> T minimum (T a, T b)
{
    return a < b ? a : b;
}

template <typename T> T maximum (T a, T b)
{
    return a > b ? a : b;
}

class Box
{
    vec2 min;
    vec2 max;

    public:

    Box():
        min(std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity()),
        max(-std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()) {}

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



#endif
