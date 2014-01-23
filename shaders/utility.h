
#ifndef UTILITY_H
#define UTILITY_H


uint round_up_div(uint n, uint d)
{
    return n/d + ((n%d == 0) ? 0 : 1);
}


#endif // UTILITY_H
