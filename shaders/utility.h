
#ifndef UTILITY_H
#define UTILITY_H

const float infinity = 999999999999999999.0;

uint round_up_div(uint n, uint d)
{
    return n/d + ((n%d == 0) ? 0 : 1);
}


bool intersects (vec3 min1, vec3 max1, vec3 min2, vec3 max2)
{
    return all(lessThan(min1, max2)) && all(lessThan(min2, max1));
}

#endif // UTILITY_H
