#pragma once

#include <type_traits>

namespace notf {

// use cool names, just in case your compiler isn't cool enough
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ulonglong = unsigned long long;

/// \brief Counts the digits in a given integral number.
template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
constexpr ushort count_digits(T digits)
{
    ushort counter = 1;
    while ((digits /= 10) >= 1) {
        ++counter;
    }
    return counter;
}
}
