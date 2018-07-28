#pragma once

#include <cfloat>

#include <functional>

#include "common/exception.hpp"
#include "common/numeric.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

constexpr long double PI = 3.141592653589793238462643383279502884197169399375105820975l;
constexpr long double KAPPA = 0.552284749830793398402251632279597438092895833835930764235l;

} // namespace detail

// constants ======================================================================================================== //

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

// operations ======================================================================================================= //

using std::atan2;
using std::cos;
using std::fmod;
using std::roundf;
using std::sin;
using std::sqrt;
using std::tan;

/// Tests whether a given value is NAN.
template<class T>
inline std::enable_if_t<std::is_floating_point<T>::value, bool> is_nan(const T& value)
{
    return std::isnan(value);
}
template<class T>
inline constexpr std::enable_if_t<std::is_integral<T>::value, bool> is_nan(const T&)
{
    return false;
}

/// Tests whether a given value is INFINITY.
template<class T>
inline std::enable_if_t<std::is_floating_point<T>::value, bool> is_inf(const T& value)
{
    return std::isinf(value);
}
template<class T>
inline constexpr std::enable_if_t<std::is_integral<T>::value, bool> is_inf(const T&)
{
    return false;
}

/// Tests whether a given value is a valid float value (not NAN, not INFINITY).
template<class T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
inline bool is_real(const T value)
{
    return !is_nan(value) && !is_inf(value);
}

/// Tests whether a given value is (almost) zero.
template<class T>
inline bool is_zero(const T value, const T epsilon = precision_high<T>())
{
    return abs(value) < epsilon;
}

/// Tests, if a value is positive or negative.
/// @return  -1 if the value is negative, 1 if it is zero or above.
template<class T>
inline T sign(const T value)
{
    return std::signbit(value) ? -1 : 1;
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
        NOTF_THROW(logic_error, "Cannot normalize an invalid number");
    }
    const T modulo = fmod(alpha, static_cast<T>(detail::PI * 2));
    return static_cast<T>(modulo >= 0 ? modulo : (detail::PI * 2) + modulo);
}

/// Save division, throws a std::logic_error if the divisor is 0.
template<class L, class R, typename T = std::common_type_t<L, R>>
inline T save_div(const L divident, const R divisor)
{
    if (divisor == 0) {
        NOTF_THROW(logic_error, "Division by zero");
    }
    return divident / divisor;
}

// approx =========================================================================================================== //

/// Test if two Reals are approximately the same value.
/// Returns true also if the difference is exactly epsilon, which allows epsilon to be zero for exact comparison.
///
/// Floating point comparison from:
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
template<class L, class R, typename T = std::common_type_t<L, R>>
bool is_approx(const L lhs, const R rhs, const T epsilon = precision_high<T>())
{
    if (is_nan(lhs) || is_nan(rhs)) {
        return false;
    }

    // if the numbers are really small, use the absolute epsilon
    if (abs(lhs - rhs) <= epsilon) {
        return true;
    }

    // use a relative epsilon if the numbers are larger
    if (abs(lhs - rhs) <= ((abs(lhs) > abs(rhs)) ? abs(lhs) : abs(rhs)) * epsilon) {
        return true;
    }

    // infinities are always approximately equal...
    if (is_inf(lhs) && is_inf(rhs)) {
        return true;
    }

    return false;
}

NOTF_CLOSE_NAMESPACE
