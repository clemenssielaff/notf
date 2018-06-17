#pragma once

#include <cfloat>
#include <cmath>

#include <functional>
#include <limits>

#include "common/exception.hpp"
#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

constexpr long double PI = 3.141592653589793238462643383279502884197169399375105820975l;
constexpr long double KAPPA = 0.552284749830793398402251632279597438092895833835930764235l;

} // namespace detail

// ================================================================================================================== //

template<class T = long double>
constexpr T pi()
{
    return static_cast<T>(detail::PI);
}

/// Length of a bezier control vector to draw a circle with radius 1.
template<class T = long double>
constexpr T kappa()
{
    return static_cast<T>(detail::KAPPA);
}

using std::abs;
using std::atan2;
using std::cos;
using std::fmod;
using std::pow;
using std::roundf;
using std::sin;
using std::sqrt;
using std::tan;

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

/// Tests whether a given value is NAN.
template<class T>
inline bool is_nan(const T value)
{
    return std::isnan(value);
}

/// Tests whether a given value is INFINITY.
template<class T>
inline bool is_inf(const T value)
{
    return std::isinf(value);
}

/// Tests whether a given value is a valid float value (not NAN, not INFINITY).
template<class T>
inline bool is_real(const T value)
{
    return !is_nan(value) && !is_inf(value);
}

/// Tests, if a value is positive or negative.
/// @return  -1 if the value is negative, 1 if it is zero or above.
template<class T>
inline T sign(const T value)
{
    return std::signbit(value) ? -1 : 1;
}

/// Clamps an input value to a given range.
template<class Value, class Min, class Max>
inline Value clamp(const Value value, const Min min, const Max max)
{
    return notf::max(static_cast<Value>(min), notf::min(static_cast<Value>(max), value));
}

/// Save asin calculation.
/// @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
template<class T>
inline T asin(const T value)
{
    return std::asin(clamp(value, -1, 1));
}

/// Save acos calculation.
/// @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
template<class T>
inline T acos(const T value)
{
    return std::acos(clamp(value, -1, 1));
}

/// Degree to Radians.
template<class T>
inline T deg_to_rad(const T degrees)
{
    return degrees * static_cast<T>(detail::PI / 180.l);
}

/// Degree to Radians.
template<class T>
inline T rad_to_deg(const T radians)
{
    return radians * static_cast<T>(180.l / detail::PI);
}

/// Normalize Radians to a value within [-pi, pi].
template<class T>
inline T norm_angle(const T alpha)
{
    if (!is_real(alpha)) {
        notf_throw(logic_error, "Cannot normalize an invalid number");
    }
    const T modulo = fmod(alpha, static_cast<T>(detail::PI * 2));
    return static_cast<T>(modulo >= 0 ? modulo : (detail::PI * 2) + modulo);
}

/// Save division, throws a std::logic_error if the divisor is 0.
template<class L, class R, typename T = std::common_type_t<L, R>>
inline T save_div(const L divident, const R divisor)
{
    if (divisor == 0) {
        notf_throw(logic_error, "Division by zero");
    }
    return divident / divisor;
}

//= precision ======================================================================================================= //

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

// approx =========================================================================================================== //

/// Test if two Reals are approximately the same value.
/// Returns true also if the difference is exactly epsilon, which allows epsilon to be zero for exact comparison.
///
/// Floating point comparison from:
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
template<class L, class R, typename T = std::common_type_t<L, R>>
bool approx(const L lhs, const R rhs)
{
    if (!is_real(lhs) || !is_real(rhs)) {
        return false;
    }

    // if the numbers are really small, use the absolute epsilon
    if (abs(lhs - rhs) <= std::numeric_limits<T>::epsilon()) {
        return true;
    }

    // use a relative epsilon if the numbers are larger
    if (abs(lhs - rhs) <= ((abs(lhs) > abs(rhs)) ? abs(lhs) : abs(rhs)) * std::numeric_limits<T>::epsilon()) {
        return true;
    }

    return false;
}

NOTF_CLOSE_NAMESPACE
