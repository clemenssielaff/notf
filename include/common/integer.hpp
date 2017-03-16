#pragma once

#include <type_traits>

#include "utils/sfinae.hpp"

namespace notf {

// use cool names, just in case your compiler isn't cool enough
using uchar     = unsigned char;
using ushort    = unsigned short;
using uint      = unsigned int;
using ulong     = unsigned long;
using ulonglong = unsigned long long;

/// @brief Counts the digits in a given integral number.
template <class Integer, ENABLE_IF_INT(Integer)>
constexpr ushort count_digits(Integer digits)
{
    ushort counter = 1;
    while ((digits /= 10) >= 1) {
        ++counter;
    }
    return counter;
}

/** Implements Python's integer modulo operation where negative values wrap around.
 * @param n     n % M
 * @param M     n % M
 * @return      n % M, while negative values are wrapped (for example -1%3=2).
 */
template <class Integer, ENABLE_IF_INT(Integer)>
constexpr Integer wrap_mod(const Integer n, const Integer M)
{
    return ((n % M) + M) % M;
}

/** Returns the next interval from a given value.
 * For example, with an interval of 60 we would get the following results:
 *     value = 0   => interval =  60
 *     value = 1   => interval =  60
 *     value = 59  => interval =  60
 *     value = 60  => interval = 120
 *     value = 61  => interval = 120
 *     ...
 */
template <class Integer, ENABLE_IF_INT(Integer)>
constexpr Integer next_interval(Integer value, const Integer interval)
{
    if (!interval) {
        return value;
    }
    else {
        value += interval;
        return value - (value % interval);
    }
}
}
