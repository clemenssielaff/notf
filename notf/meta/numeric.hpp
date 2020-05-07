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
template<class T>
constexpr T exp(const T& number, uint exponent) noexcept {
    if (exponent == 0) { return 1; }
    T result = number;
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
/// There exists no value `x` of the type for which `x > highest_v<T>` is true.
template<class T>
static constexpr const T highest_v = std::numeric_limits<T>::max();

/// Lowest value representable with the given type.
/// There exists no value `x` of the type for which `x < lowest_v<T>` is true.
template<class T>
static constexpr const T lowest_v = std::numeric_limits<T>::lowest();

/// Helper struct containing the type that has a higher numeric limits.
template<class LEFT, class RIGHT>
struct higher_type {
    using type = std::conditional_t<(highest_v<LEFT> <= highest_v<RIGHT>), LEFT, RIGHT>;
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
template<class Target, class Source>
constexpr bool can_be_narrow_cast(const Source& source) noexcept {
    if constexpr (std::is_same_v<Source, Target>) {
        // both are the same type
        return true;
    } else if constexpr (std::conjunction_v<std::is_integral<Source>, std::is_integral<Target>>) {
        // both are integers
        if constexpr (is_same_signedness_v<Source, Target>) {
            // both have the same signedness
            if constexpr (std::is_signed_v<Source>) {
                // both are signed
                if constexpr (sizeof(Target) >= sizeof(Source)) {
                    // target type is larger or equal
                    return true;
                } else {
                    // source type is larger
                    return (static_cast<Source>(highest_v<Target>) <= source)
                           && (static_cast<Source>(lowest_v<Target>) >= source);
                }
            } else {
                // both are unsigned
                if constexpr (sizeof(Target) >= sizeof(Source)) {
                    // target type is larger or equal
                    return true;
                } else {
                    // source type is larger
                    return source <= static_cast<Source>(highest_v<Target>);
                }
            }
        } else {
            // source and targed have different signedness
            if constexpr (std::is_signed_v<Source>) {
                // source is signed, target is unsigned
                if (source < 0) {
                    // source is negative
                    return false;
                } else {
                    // source is positive
                    if constexpr (sizeof(Target) >= sizeof(Source)) {
                        // target type is larger or equal
                        return true;
                    } else {
                        // source type is larger
                        return source <= static_cast<Source>(highest_v<Target>);
                    }
                }
            } else {
                // target is signed, source is unsigned
                if constexpr (sizeof(Target) >= sizeof(Source)) {
                    // target type is larger or equal
                    return true;
                } else {
                    // source type is larger
                    return source <= static_cast<Source>(highest_v<Target>);
                }
            }
        }
    } else if constexpr (std::conjunction_v<std::is_floating_point<Source>, std::is_floating_point<Target>>) {
        // both are floating point
        if constexpr (sizeof(Target) >= sizeof(Source)) {
            // target type is larger or equal
            return true;
        } else {
            // source type is larger
            return (static_cast<Source>(highest_v<Target>) <= source)
                   && (static_cast<Source>(lowest_v<Target>) >= source);
        }
    } else if constexpr (std::conjunction_v<is_numeric<Source>, is_numeric<Target>>) {
        if constexpr (std::is_integral_v<Source>) {
            // source is integral, target is floating point
            return abs(source) <= exp(std::numeric_limits<Target>::radix, std::numeric_limits<Target>::digits);
        } else {
            // source is floating point, target is integral
            if constexpr (std::is_signed_v<Target>) {
                // target is signed
                return static_cast<Source>(static_cast<Target>(source)) == source;
            } else {
                // target is unsigned
                if (source < 0) {
                    // source is negative
                    return false;
                } else {
                    // source is positive
                    return static_cast<Source>(static_cast<Target>(source)) == source;
                }
            }
        }
    } else {
        // non-numeric conversion
        return false;
    }
}

/// Save narrowing cast.
/// Original inspiration from:
///     https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class Target, class Source>
constexpr Target narrow_cast(const Source& source) {
    if (can_be_narrow_cast<Target, Source>(source)) {
        return static_cast<Target>(source);
    } else {
        NOTF_THROW(ValueError, "narrow_cast failed");
    }
}

static_assert(can_be_narrow_cast<int>(int(0)));

NOTF_CLOSE_NAMESPACE
