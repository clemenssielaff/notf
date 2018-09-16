#pragma once

#include <limits>
#include <utility>

#include "./meta.hpp"

NOTF_OPEN_META_NAMESPACE

// short unsigned integer names ===================================================================================== //

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ulonglong = unsigned long long;

// operations ======================================================================================================= //

/// Variadic min using auto type deduction.
template<class T>
constexpr T&& min(T&& val) noexcept
{
    return std::forward<T>(val);
}
template<class L, class R, typename... Tail, typename T = std::common_type_t<L, R>>
constexpr T min(L&& lhs, R&& rhs, Tail&&... tail) noexcept
{
    const T rest = min(rhs, std::forward<Tail>(tail)...);
    return rest < static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// Variadic max using auto type deduction.
template<class T>
constexpr T&& max(T&& val) noexcept
{
    return std::forward<T>(val);
}
template<class L, class R, typename... Tail, typename T = std::common_type_t<L, R>>
constexpr T max(L&& lhs, R&& rhs, Tail&&... tail) noexcept
{
    const T rest = max(rhs, std::forward<Tail>(tail)...);
    return rest > static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// Clamps an input value to a given range.
template<class T, class Min, class Max>
constexpr T clamp(const T value, const Min min, const Max max) noexcept
{
    return max(static_cast<T>(min), min(static_cast<T>(max), value));
}

/// Calculates number^exponent.
template<class T>
constexpr T exp(const T number, uint exponent) noexcept
{
    if (exponent == 0) {
        return 1;
    }
    T result = number;
    while (exponent-- > 1) {
        result *= number;
    }
    return result;
}

/// Returns the nth digit from the right.
/// Digit #0 is the least significant digit.
template<std::size_t base = 10>
constexpr std::size_t get_digit(const std::size_t number, const std::size_t digit) noexcept
{
    static_assert(base > 1);
    return (number % exp(base, digit + 1)) / exp(base, digit);
}

/// Counts the digits in an integral number.
template<std::size_t base = 10>
constexpr std::size_t count_digits(std::size_t number) noexcept
{
    static_assert(base > 1);
    std::size_t result = 1;
    while (number /= base) {
        ++result;
    }
    return result;
}

// limits =========================================================================================================== //

/// Highest value representable with the given type.
/// There exists no value X of the type for which "X > max_value<T>()" is true.
template<class T>
constexpr T max_value() noexcept
{
    return std::numeric_limits<T>::max();
}

/// Lowest value representable with the given type.
/// There exists no value X of the type for which "X < min_value<T>()" is true.
template<class T>
constexpr T min_value() noexcept
{
    return std::numeric_limits<T>::lowest();
}

/// Helper struct containing the type that has a higher numeric limits.
template<typename LEFT, typename RIGHT>
struct higher_type {
    using type = typename std::conditional<max_value<LEFT>() <= max_value<RIGHT>(), LEFT, RIGHT>::type;
};

// precision ======================================================================================================== //

/// Type dependent constant for low-precision approximation (useful for use in "noisy" functions).
/// Don't be fooled by the name though, "low" precision is still pretty precise on a human scale.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, T> precision_low() noexcept
{
    return std::numeric_limits<T>::epsilon() * 100;
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, T> precision_low() noexcept
{
    return 0; // integers have no leeway
}

/// Type dependent constant for high-precision approximation.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, T> precision_high() noexcept
{
    return std::numeric_limits<T>::epsilon() * 3;
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, T> precision_high() noexcept
{
    return 0; // integers have no leeway
}

NOTF_CLOSE_META_NAMESPACE
