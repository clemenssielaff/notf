#pragma once

#include <limits>
#include <utility>

#include "notf/meta/array.hpp"
#include "notf/meta/config.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// operations ======================================================================================================= //

/// Absolute value.
template<class T>
constexpr T abs(T val) noexcept {
    return val >= 0 ? val : -val;
}

/// Variadic min using auto type deduction.
template<class T>
constexpr T&& min(T&& val) noexcept {
    return std::forward<T>(val);
}
template<class L, class R, class... Tail, class T = std::common_type_t<L, R>>
constexpr T min(L lhs, R rhs, Tail&&... tail) noexcept {
    const T rest = min(rhs, std::forward<Tail>(tail)...);
    return rest < static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// In-place min
template<class L, class R, class... Tail>
constexpr void set_min(L& lhs, R rhs, Tail&&... tail) noexcept {
    lhs = min(lhs, static_cast<L>(rhs), std::forward<Tail>(tail)...);
}

/// Variadic max using auto type deduction.
template<class T>
constexpr T&& max(T&& val) noexcept {
    return std::forward<T>(val);
}
template<class L, class R, class... Tail, class T = std::common_type_t<L, R>>
constexpr T max(L lhs, R rhs, Tail&&... tail) noexcept {
    const T rest = max(static_cast<T>(rhs), std::forward<Tail>(tail)...);
    return rest > static_cast<T>(lhs) ? rest : static_cast<T>(lhs); // returns lhs if both are equal
}

/// In-place max
template<class L, class R, class... Tail>
constexpr void set_max(L& lhs, R rhs, Tail&&... tail) noexcept {
    lhs = max(lhs, static_cast<L>(rhs), std::forward<Tail>(tail)...);
}

/// Clamps an input value to a given range.
template<class T>
constexpr T clamp(const T value, const identity_t<T> min = 0, const identity_t<T> max = 1) noexcept {
    return notf::max(min, notf::min(max, value));
}

/// Calculates number^exponent.
template<class T, class Out = std::decay_t<T>>
constexpr Out exp(T&& number, uint exponent) noexcept {
    if (exponent == 0) { return 1; }
    Out result = number;
    while (exponent-- > 1) {
        result *= number;
    }
    return result;
}

/// Produces the sum of all given arguments.
/// In a template function, this might well produce less assembly than a loop.
template<class... Ts>
constexpr auto sum(Ts&&... ts) {
    return (std::forward<Ts>(ts) + ...);
}

// limits =========================================================================================================== //

/// Highest value representable with the given type.
/// There exists no value X of the type for which "X > max_value<T>()" is true.
template<class T>
constexpr T max_value() noexcept {
    return std::numeric_limits<T>::max();
}

/// Lowest value representable with the given type.
/// There exists no value X of the type for which "X < min_value<T>()" is true.
template<class T>
constexpr T min_value() noexcept {
    return std::numeric_limits<T>::lowest();
}

/// Helper struct containing the type that has a higher numeric limits.
template<class LEFT, class RIGHT>
struct higher_type {
    using type = std::conditional_t<(max_value<LEFT>() <= max_value<RIGHT>()), LEFT, RIGHT>;
};

// precision ======================================================================================================== //

/// Type dependent constant for low-precision approximation (useful for use in "noisy" functions).
/// Don't be fooled by the name though, "low" precision is still pretty precise on a human scale.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, T> precision_low() noexcept {
    return std::numeric_limits<T>::epsilon() * 100;
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, T> precision_low() noexcept {
    return 0; // integers have no leeway
}

/// Type dependent constant for high-precision approximation.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, T> precision_high() noexcept {
    return std::numeric_limits<T>::epsilon() * 3;
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, T> precision_high() noexcept {
    return 0; // integers have no leeway
}

// power list ======================================================================================================= //

/// Returns the first N powers of the given base value.
/// Given `x` returns an array {1, x, x^2, x^3, ..., x^N}
template<ulong N, class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr std::array<T, N> power_list(const T& x) noexcept {
    T result[N]{};
    result[0] = 1;
    for (ulong i = 1; i < N; ++i) {
        result[i] = result[i - 1] * x;
    }
    return to_array(result);
}

// narrow cast ====================================================================================================== //

/// Tests if a value can be narrow cast and optionally passes the cast value out again.
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr bool can_be_narrow_cast(Source&& value, target_t& result) noexcept {
    // skip the check if both types are the same
    if constexpr (std::is_same_v<source_t, target_t>) {
        result = std::forward<Source>(value);

        // TODO: skip expensive testing if target type "subsumes" source type
    } else {
        // simple reverse check
        result = static_cast<target_t>(std::forward<Source>(value));
        if (static_cast<source_t>(result) != value) { return false; }

        // check sign overflow
        if constexpr (!is_same_signedness<target_t, source_t>::value) {
            if ((result < 0) != (value < 0)) { return false; }
        }
    }
    return true;
}
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr bool can_be_narrow_cast(Source&& value) noexcept {
    static_assert(std::is_nothrow_constructible_v<target_t>);
    target_t ignored;
    return can_be_narrow_cast(std::forward<Source>(value), ignored);
}

/// Save narrowing cast.
/// https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class target_t, class Source, class source_t = std::decay_t<Source>>
constexpr target_t narrow_cast(Source&& value) {
    if (target_t result; can_be_narrow_cast(std::forward<Source>(value), result)) { return result; }
    NOTF_THROW(ValueError, "narrow_cast failed");
}

NOTF_CLOSE_NAMESPACE
