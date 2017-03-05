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

/** Clamps an integer value into a given range. */
template <class Integer, ENABLE_IF_INT(Integer)>
constexpr Integer clamp(const Integer value, const Integer min, const Integer max)
{
    return value > max ? max : (value < min ? min : value);
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
}
