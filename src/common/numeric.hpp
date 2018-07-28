#pragma once

#include <cmath>
#include <limits>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

// operations ======================================================================================================= //

using std::abs;
using std::pow;

/// Variadic min using auto type deduction.
template<class T>
constexpr T&& min(T&& val)
{
    return std::forward<T>(val);
}
template<class L, class R, typename... TAIL, typename T = std::common_type_t<L, R>>
constexpr T min(L&& lhs, R&& rhs, TAIL&&... tail)
{
    const T rest = min(rhs, std::forward<TAIL>(tail)...);
    return rest < static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// Variadic max using auto type deduction.
template<class T>
constexpr T&& max(T&& val)
{
    return std::forward<T>(val);
}
template<class L, class R, typename... TAIL, typename T = std::common_type_t<L, R>>
constexpr T max(L&& lhs, R&& rhs, TAIL&&... tail)
{
    const T rest = max(rhs, std::forward<TAIL>(tail)...);
    return rest > static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// Clamps an input value to a given range.
template<class Value, class Min, class Max>
inline Value clamp(const Value value, const Min min, const Max max)
{
    return notf::max(static_cast<Value>(min), notf::min(static_cast<Value>(max), value));
}

// limits =========================================================================================================== //

/// Highest value representable with the given type.
/// There exists no value X of the type for which "X > max_value<T>()" is true.
template<class T>
constexpr T max_value()
{
    return std::numeric_limits<T>::max();
}

/// Lowest value representable with the given type.
/// There exists no value X of the type for which "X < min_value<T>()" is true.
template<class T>
constexpr T min_value()
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
constexpr T precision_low();

template<>
constexpr float precision_low<float>()
{
    return std::numeric_limits<float>::epsilon() * 100;
}

template<>
constexpr double precision_low<double>()
{
    return std::numeric_limits<double>::epsilon() * 100;
}

template<>
constexpr int precision_low<int>()
{
    return 0;
}

/// Type dependent constant for high-precision approximation.
template<class T>
constexpr T precision_high();

template<>
constexpr float precision_high<float>()
{
    return std::numeric_limits<float>::epsilon() * 3;
}

template<>
constexpr double precision_high<double>()
{
    return std::numeric_limits<double>::epsilon() * 3;
}

template<>
constexpr short precision_high<short>()
{
    return 0;
}

template<>
constexpr int precision_high<int>()
{
    return 0;
}

NOTF_CLOSE_NAMESPACE
