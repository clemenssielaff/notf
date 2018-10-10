#pragma once

#include <limits>
#include <utility>

#include "./config.hpp"
#include "./exception.hpp"
#include "./types.hpp"

NOTF_OPEN_NAMESPACE

// short unsigned integer names ===================================================================================== //

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

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
template<class T, class Out = std::decay_t<T>>
constexpr Out exp(T&& number, uint exponent) noexcept
{
    if (exponent == 0) {
        return 1;
    }
    Out result = number;
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

// narrow cast ====================================================================================================== //

/// Tests if a value can be narrow cast and optionally passes the cast value out again.
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr bool can_be_narrow_cast(Source&& value, target_t& result) noexcept
{
    // skip the check if both types are the same
    if constexpr (std::is_same_v<source_t, target_t>) {
        result = std::forward<Source>(value);

        // TODO: skip expensive testing if target type "subsumes" source type
    }
    else {
        // simple reverse check
        result = static_cast<target_t>(std::forward<Source>(value));
        if (static_cast<source_t>(result) != value) {
            return false;
        }

        // check sign overflow
        if constexpr (!is_same_signedness<target_t, source_t>::value) {
            if ((result < 0) != (value < 0)) {
                return false;
            }
        }
    }
    return true;
}
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr bool can_be_narrow_cast(Source&& value) noexcept
{
    static_assert(std::is_nothrow_constructible_v<target_t>);
    target_t ignored;
    return can_be_narrow_cast(std::forward<Source>(value), ignored);
}

/// Save narrowing cast.
/// https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr target_t narrow_cast(Source&& value)
{
    if (target_t result; can_be_narrow_cast(std::forward<Source>(value), result)) {
        return result;
    }
    NOTF_THROW(value_error, "narrow_cast failed");
}

// corresponding types ============================================================================================== //

/// Signed integer type corresponding to an unsigned one.
template<class T, class = std::enable_if_t<std::is_integral_v<T> && !std::is_signed_v<T>>>
struct corresponding_signed {
    static_assert(always_false_v<T>);
};
template<>
struct corresponding_signed<uchar> {
    using type = char;
};
template<>
struct corresponding_signed<ushort> {
    using type = short;
};
template<>
struct corresponding_signed<uint> {
    using type = int;
};
template<>
struct corresponding_signed<ulong> {
    using type = long;
};
template<class T>
using corresponding_signed_t = typename corresponding_signed<T>::type;

/// Unsigned integer type corresponding to a signed one.
template<class T, class = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
struct corresponding_unsigned {
    static_assert(always_false_v<T>);
};
template<>
struct corresponding_unsigned<char> {
    using type = uchar;
};
template<>
struct corresponding_unsigned<short> {
    using type = ushort;
};
template<>
struct corresponding_unsigned<int> {
    using type = uint;
};
template<>
struct corresponding_unsigned<long> {
    using type = ulong;
};
template<class T>
using corresponding_unsigned_t = typename corresponding_unsigned<T>::type;

NOTF_CLOSE_NAMESPACE
