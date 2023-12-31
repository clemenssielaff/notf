#pragma once

#include <cmath>

#include "notf/meta/numeric.hpp"

NOTF_OPEN_NAMESPACE

// templated real types ============================================================================================= //

template<std::size_t size>
struct templated_real;
template<>
struct templated_real<32> {
    using type = float;
};
template<>
struct templated_real<8> {
    using type = double;
};
template<std::size_t size>
using templated_real_t = typename templated_real<size>::type;

// constants ======================================================================================================== //

/// Pi
template<class T = long double>
constexpr T pi() noexcept {
    return static_cast<T>(3.141592653589793238462643383279502884197169399375105820975l);
}

/// Length of a bezier control vector to draw a circle with radius 1.
template<class T = long double>
constexpr T kappa() noexcept {
    return static_cast<T>(0.552284749830793398402251632279597438092895833835930764235l);
}

/// The golden ratio, approx (sqrt(5) + 1) / 2).
template<class T = long double>
constexpr T phi() noexcept {
    return static_cast<T>(1.618033988749894848204586834365638117720309179805762862135l);
}

// operations ======================================================================================================= //

using std::cos;
using std::fmod;
using std::pow;
using std::roundf;
using std::sin;
using std::sqrt;
using std::tan;

/// @{
/// Fast trigonometric function approximations for small angles.
/// Thresholds are set at 1% relative error, as described here:
/// https://en.wikipedia.org/wiki/Small-angle_approximation#Error_of_the_approximations
template<class T>
T fast_sin(const T& radians) {
    if (radians < 0.244) {
        return radians;
    } else {
        return sin(radians);
    }
}

template<class T>
T fast_cos(const T& radians) {
    if (radians < 0.664) {
        return T(1) - (radians * radians) / T(2);
    } else {
        return cos(radians);
    }
}

template<class T>
T fast_tan(const T& radians) {
    if (radians < 0.176) {
        return radians;
    } else {
        return tan(radians);
    }
}
/// @}

/// Tests whether a given value is NAN.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> is_nan(const T value) noexcept {
    return value != value;
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, bool> is_nan(const T) noexcept {
    return false;
}

/// Tests whether a given value is INFINITY.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> is_inf(const T value) noexcept {
    if constexpr (!std::numeric_limits<T>::has_infinity) { return false; }
    return value == std::numeric_limits<T>::infinity();
}
template<class T>
constexpr std::enable_if_t<std::is_integral_v<T>, bool> is_inf(const T) noexcept {
    return false;
}

/// Tests whether a given value is a valid float value (not NAN, not INFINITY).
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> is_real(const T value) noexcept {
    return !is_nan(value) && !is_inf(value);
}

/// @{
/// Tests whether a given value is (almost) a zero float value.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> is_zero(const T value) noexcept {
    return std::fpclassify(value) == FP_ZERO;
}
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, bool> is_zero(const T value,
                                                                      const identity_t<T> epsilon) noexcept {
    return abs(value) < epsilon;
}
/// @}

/// Tests, if a value is positive or negative.
/// @return  -1 if the value is negative, 1 if it is zero or above.
template<class T>
constexpr T sign(const T value) {
    return std::signbit(value) ? -1 : 1;
}

/// Save asin calculation.
/// @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
template<class T>
constexpr T asin(const T value) {
    return std::asin(clamp(value, -1, 1));
}

/// Save acos calculation.
/// @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
template<class T>
constexpr T acos(const T value) {
    return std::acos(clamp(value, -1, 1));
}

/// Degree to Radians.
template<class T>
constexpr T deg_to_rad(const T degrees) noexcept {
    return degrees * static_cast<T>(pi() / 180.l);
}

/// Degree to Radians.
template<class T>
constexpr T rad_to_deg(const T radians) noexcept {
    return radians * static_cast<T>(180.l / pi());
}

/// Normalize Radians to a value within [-pi, pi].
template<class T>
T norm_angle(const T alpha) {
    const T modulo = fmod(alpha, static_cast<T>(pi() * 2));
    return static_cast<T>(modulo >= 0 ? modulo : (pi() * 2) + modulo);
}

/// Next representable floating point number greater than the given one.
template<class T>
constexpr std::enable_if_t<std::is_floating_point_v<T>, T> next_after(const T value) noexcept {
    return std::nextafter(value, max_v<T>);
}

/// Breaks a given real number into an integral and fractional part.
/// @param real     Real number to break.
/// @param integer
/// @returns        Fractional part of the given real.
template<class I = long, class R = double,
         class = std::enable_if_t<std::conjunction_v<std::is_floating_point<R>, std::is_integral<I>>>>
std::pair<I, R> break_real(const R& real) {
    double intpart;
    R result = std::modf(real, &intpart);
    return std::make_pair(static_cast<I>(intpart), result);
}

/// @{
/// Fast inverse square.
/// From https://stackoverflow.com/a/41637260/
/// Magic numbers from https://cs.uwaterloo.ca/~m32rober/rsqrt.pdf
inline float fast_inv_sqrt(float number) noexcept {
    const float temp = number * 0.5f;

    std::uint32_t& i = *std::launder(reinterpret_cast<std::uint32_t*>(&number));
    i = 0x5f375a86 - (i >> 1);
    number = *std::launder(reinterpret_cast<float*>(&i));

    return number * (1.5f - (temp * number * number));
}
inline double fast_inv_sqrt(double number) noexcept {
    const double temp = number * 0.5;

    std::uint64_t& i = *std::launder(reinterpret_cast<std::uint64_t*>(&number));
    i = 0x5fe6eb50c7b537a9 - (i >> 1);
    number = *std::launder(reinterpret_cast<double*>(&i));

    return number * (1.5 - (temp * number * number));
}
/// @}

/// Smoothstep function as implemented in GLSL.
/// From https://en.wikipedia.org/wiki/Smoothstep
template<class T>
T smoothstep(T x, const T lower_bound = 0, const T upper_bound = 1) {
    x = clamp((x - lower_bound) / (upper_bound - lower_bound), 0, 1);
    return x * x * (3 - 2 * x);
}

/// More expensive smoothstep function, but with zero 1st- and 2nd-order derivates at x=0 and x=1.
template<class T>
T smootherstep(T x, const T lower_bound = 0, const T upper_bound = 1) {
    x = clamp((x - lower_bound) / (upper_bound - lower_bound), 0, 1);
    return x * x * x * (x * (x * 6 - 15) + 10);
}

/// Linear interpolation between two arithmetic values.
/// @param from    Left value, full weight at blend <= 0.
/// @param to      Right value, full weight at blend >= 1.
/// @param blend   Blend value, clamped to range [0, 1].
template<class T, class R>
constexpr T lerp(const T& from, const T& to, const R& blend) noexcept {
    if (blend <= 0) {
        return from;
    } else if (blend >= 1) {
        return to;
    }
    return ((to - from) *= blend) += from;
}

// approx =========================================================================================================== //

/// Test if two Reals are approximately the same value.
/// Returns true also if the difference is exactly epsilon, which allows epsilon to be zero for exact comparison.
///
/// Floating point comparison from:
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
template<class L, class R, typename T = std::common_type_t<L, R>>
constexpr bool is_approx(const L lhs, const R rhs, const T epsilon = precision_high<T>()) noexcept {
    // if only one argument is infinite, the other one cannot be approximately the same
    if constexpr (std::is_integral_v<L>) {
        if (is_nan(rhs) || is_inf(rhs)) { return false; }
    } else if constexpr (std::is_integral_v<R>) {
        if (is_nan(lhs) || is_inf(lhs)) { return false; }
    } else {
        // nans are never approximately equal
        if (is_nan(lhs) || is_nan(rhs)) { return false; }
        if (is_inf(lhs)) {
            if (is_inf(rhs)) {
                return true; // infinities are always approximately equal
            } else {
                return false;
            }
        } else if (is_inf(rhs)) {
            return false;
        }
    }

    // if the numbers are really small, use the absolute epsilon
    if (abs(lhs - rhs) <= epsilon) { return true; }

    // use a relative epsilon if the numbers are larger
    if (abs(lhs - rhs) <= ((abs(lhs) > abs(rhs)) ? abs(lhs) : abs(rhs)) * epsilon) { return true; }

    return false;
}

// literals ========================================================================================================= //

NOTF_OPEN_LITERALS_NAMESPACE

/// Floating point literal to convert degrees to radians.
constexpr long double operator"" _deg(long double degrees) { return deg_to_rad(degrees); }

/// Integer literal to convert degrees to radians.
constexpr long double operator"" _deg(unsigned long long int degrees) {
    return deg_to_rad(static_cast<long double>(degrees));
}

NOTF_CLOSE_LITERALS_NAMESPACE
NOTF_CLOSE_NAMESPACE
