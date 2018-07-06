#pragma once

#include <numeric>

#include "common/exception.hpp"
#include "common/numeric.hpp"

NOTF_OPEN_NAMESPACE

/// Counts the digits in a given integral number.
template<class Integer, typename = notf::enable_if_t<std::is_integral<Integer>::value>>
inline constexpr ushort count_digits(Integer digits)
{
    ushort counter = 1;
    while ((digits /= 10) >= 1) {
        ++counter;
    }
    return counter;
}

/// Implements Python's integer modulo operation where negative values wrap around.
/// @param n     n % M
/// @param M     n % M
/// @return      n % M, while negative values are wrapped (for example -1%3=2).
template<class Integer, typename = notf::enable_if_t<std::is_integral<Integer>::value>>
inline constexpr Integer wrap_mod(const Integer n, const Integer M)
{
    return ((n % M) + M) % M;
}

/// Returns the next interval from a given value.
/// For example, with an interval of 60 we would get the following results:
///     value = 0   => interval =  60
///     value = 1   => interval =  60
///     value = 59  => interval =  60
///     value = 60  => interval = 120
///     value = 61  => interval = 120
///     ...
template<class Integer, typename = notf::enable_if_t<std::is_integral<Integer>::value>>
inline constexpr Integer next_interval(Integer value, const Integer interval)
{
    if (!interval) {
        return value;
    }
    else {
        value += interval;
        return value - (value % interval);
    }
}

/// Calculate the Greatest Common Divisor of two integers.
/// The GDC is the largest positive integer that divides each of the integers
/// @throws value_error If one or both numbers are zero.
template<class L, class R, typename T = std::common_type_t<L, R>,
         typename = std::enable_if_t<std::is_integral<T>::value>>
inline constexpr T gcd(const L& lhs, const R& rhs)
{
    T a = static_cast<T>(lhs);
    T b = static_cast<T>(rhs);
    if (a == 0 || b == 0) {
        NOTF_THROW(value_error, "Cannot calculate the GCD of {} and {}", lhs, rhs);
    }

    while (true) {
        if (a == 0) {
            return b;
        }
        b %= a;

        if (b == 0) {
            return a;
        }
        a %= b;
    }
}

/// Calculates the Least Common Multiple of two or more integers.
/// @throws value_error If any given integer is zero.
template<class L, class R, typename T = std::common_type_t<L, R>,
         typename = std::enable_if_t<std::is_integral<T>::value>>
inline constexpr T lcm(const L& lhs, const R& rhs)
{
    return abs(lhs * rhs) / gcd(lhs, rhs);
}
template<class L, class R, class... Args>
inline constexpr auto lcm(const L& lhs, const R& rhs, Args&&... args)
{
    return lcm(lhs, lcm(rhs, std::forward<Args>(args)...));
}
template<class T>
inline constexpr auto lcm(const T& numbers)
{
    return std::accumulate(std::begin(numbers), std::end(numbers), 1,
                           [](const auto& a, const auto& b) { return abs(a * b) / gcd(a, b); });
}

NOTF_CLOSE_NAMESPACE
