#ifndef UTILITY_H
#define UTILITY_H

#include "common.h"

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

#endif
