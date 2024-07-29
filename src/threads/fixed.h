#include "threads/thread.h"

#define FRACTION (1 << 14)

int intTofloat(int n)
{
    return n * FRACTION;
}

int floatToint(int x)
{
    return x / FRACTION;
}

int floatTointRound(int x)
{
    if (x >= 0)
        return (x + FRACTION / 2) / FRACTION;
    else
        return (x - FRACTION / 2) / FRACTION;
}

int float_addition(int x, int y)
{
    return x + y;
}

int float_subtraction(int x, int y)
{
    return x - y;
}

int float_addition_int(int x, int n)
{
    return x + n * FRACTION;
}

int float_subtraction_int(int x, int n)
{
    return x - n * FRACTION;
}

int float_multiple(int x, int y)
{
    return ((int64_t)x) * y / FRACTION;
}

int float_multiple_int(int x, int n)
{
    return x * n;
}

int float_divide(int x, int y)
{
    return ((int64_t)x) * FRACTION / y;
}

int float_divide_int(int x, int n)
{
    return x / n;
}