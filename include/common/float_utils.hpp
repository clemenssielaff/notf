#pragma once

#include <cfloat>
#include <cmath>
#include <functional>
#include <limits>

#include "utils/sfinae.hpp"

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif
const long double HALF_PI = 1.570796326794896619231321691639751442098584699687552910487l;
const long double PI      = 3.141592653589793238462643383279502884197169399375105820975l;
const long double TWO_PI  = 6.283185307179586476925286766559005768394338798750211641949l;
const long double KAPPA   = 0.552284749830793398402251632279597438092895833835930764235l;
#ifdef __clang__
#pragma clang diagnostic pop
#endif

using std::abs;
using std::max;
using std::min;
using std::sqrt;
using std::fmod;
using std::sin;
using std::cos;
using std::tan;
using std::atan2;
using std::roundf;

/** Tests whether a given value is NAN. */
template <typename Real>
inline bool is_nan(Real&& value) { return std::isnan(std::forward<Real>(value)); }

/** Tests whether a given value is INFINITY. */
template <typename Real>
inline bool is_inf(Real&& value) { return std::isinf(std::forward<Real>(value)); }

/** Tests whether a given value is a valid float value (not NAN, not INFINITY). */
template <typename Real>
inline bool is_real(Real&& value) { return !is_nan(value) && !is_inf(value); }

/** Tests, if a value is positive or negative.
 * @return  -1 if the value is negative, 1 if it is zero or above.
 */
template <typename Real>
inline typename std::remove_reference<Real>::type sign(Real&& value) { return std::signbit(value) ? -1 : 1; }

/** Clamps an input value to a given range. */
template <typename Value, typename Min, typename Max>
inline Value clamp(const Value value, const Min min, const Max max)
{
    return notf::max(static_cast<Value>(min), notf::min(static_cast<Value>(max), value));
}

/** Save asin calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
template <typename Real>
inline Real asin(Real&& value) { return std::asin(clamp(std::forward<Real>(value), -1, 1)); }

/** Save acos calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
template <typename Real>
inline Real acos(Real&& value) { return std::acos(clamp(std::forward<Real>(value), -1, 1)); }

/** Degree to Radians. */
template <typename Real>
inline Real deg_to_rad(Real&& degrees) { return degrees * (PI / 180.l); }

/** Degree to Radians. */
template <typename Real>
inline Real rad_to_deg(Real&& radians) { return radians * (180.l / PI); }

/** Normalize Radians to a value within [-pi, pi). */
template <typename Real>
inline Real norm_angle(Real alpha)
{
    while (alpha < 0) {
        alpha += TWO_PI;
    }
    return fmod(alpha + PI, TWO_PI) - PI;
}

// precision **********************************************************************************************************/

/** Type dependent constant for low-precision approximation (useful for use in "noisy" functions).
 * Don't be fooled by the name though, "low" precision is still pretty precise on a human scale.
 */
template <typename Type>
constexpr Type precision_low();

template <>
constexpr float precision_low<float>() { return std::numeric_limits<float>::epsilon() * 100; }

template <>
constexpr double precision_low<double>() { return std::numeric_limits<double>::epsilon() * 100; }

/** Type dependent constant for high-precision approximation. */
template <typename Type>
constexpr Type precision_high();

template <>
constexpr float precision_high<float>() { return std::numeric_limits<float>::epsilon(); }

template <>
constexpr double precision_high<double>() { return std::numeric_limits<double>::epsilon(); }

// approx *************************************************************************************************************/

/** Test if two Reals are approximately the same value.
 * Returns true also if the difference is exactly epsilon, which allows epsilon to be zero for exact comparison.
 * Note that, regardless of the signature, comparison to an _approx object is destructive and definetly NOT const.
 * Use only as directed and don't keep it around for multiple comparisons.
 *
 * Example:
 *
 * ```
 * bool is_approx = 1.1234 == approx(1.2345, 0.1);
 * ```
 *
 * Approx-class idea from catch:
 * https://github.com/philsquared/Catch/blob/master/include/catch.hpp
 *
 * Floating point comparison from:
 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _approx {

    /** Value to compare against. */
    mutable Real value;

    /** Smallest difference which is still considered equal. */
    mutable Real epsilon;

    _approx(Real value, Real epsilon)
        : value(value), epsilon(abs(epsilon)) {}

    template <typename Other, ENABLE_IF_REAL(Other)>
    friend bool operator==(Other b, const _approx& a)
    {
        if (!is_real(a.value) || !is_real(b)) {
            return false;
        }

        a.epsilon = max(static_cast<Other>(a.epsilon), std::numeric_limits<Other>::epsilon());

        // if the numbers are really small, use the absolute epsilon
        const Real diff = abs(a.value - b);
        if (diff <= a.epsilon) {
            return true;
        }

        // use a relative epsilon if the numbers are larger
        a.value = abs(a.value);
        b       = abs(b);
        if (diff <= ((a.value > b) ? a.value : b) * a.epsilon) {
            return true;
        }

        return false;
    }

    template <typename Other, ENABLE_IF_REAL(Other)>
    friend bool operator==(const _approx& lhs, Other rhs) { return operator==(rhs, lhs); }

    template <typename Other, ENABLE_IF_REAL(Other)>
    friend bool operator!=(Other lhs, const _approx& rhs) { return !operator==(lhs, rhs); }

    template <typename Other, ENABLE_IF_REAL(Other)>
    friend bool operator!=(const _approx& lhs, Other rhs) { return !operator==(rhs, lhs); }
};

template <typename Real, ENABLE_IF_REAL(Real)>
auto approx(const Real value)
{
    return _approx<Real>(value, precision_high<Real>());
}

template <typename Real, ENABLE_IF_REAL(Real)>
auto approx(const Real value, const Real epsilon)
{
    return _approx<Real>(value, epsilon);
}

template <typename Integer, ENABLE_IF_INT(Integer), DISABLE_IF_REAL(Integer)>
auto approx(const Integer value)
{
    return _approx<double>(static_cast<double>(value), precision_high<double>());
}

template <typename Real, typename Integer, ENABLE_IF_REAL(Real), ENABLE_IF_INT(Integer)>
auto approx(const Real value, const Integer epsilon)
{
    return _approx<Real>(value, static_cast<Real>(epsilon));
}

template <typename Real, typename Integer, ENABLE_IF_REAL(Real), ENABLE_IF_INT(Integer)>
auto approx(const Integer value, const Real epsilon)
{
    return _approx<Real>(static_cast<Real>(value), epsilon);
}

template <typename Integer, ENABLE_IF_INT(Integer), DISABLE_IF_REAL(Integer)>
auto approx(const Integer value, const Integer epsilon)
{
    return _approx<double>(static_cast<double>(value), static_cast<double>(epsilon));
}

} // namespace notf
