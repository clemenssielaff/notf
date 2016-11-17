#pragma once

#include <cfloat>
#include <cmath>
#include <functional>

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif
static const float HALF_PI = 1.570796326794896619231321691639751442098584699687552910487472f;
static const float PI = 3.141592653589793238462643383279502884197169399375105820974944f;
static const float TWO_PI = 6.283185307179586476925286766559005768394338798750211641949889f;
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

/** Tests, if a value is positive or negative.
 * @return  -1 if the value is negative, 1 if it is zero or above.
 */
inline float sign(const float value) { return std::signbit(value) ? -1 : 1; }

/** Clamps an input value to a given range. */
inline float clamp(const float value, const float min, const float max)
{
    return value < min ? min : (value > max ? max : value);
}

/** Test if two Reals are approximately the same value.
 * From https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 * approx() returns true also if the difference is exactly epsilon.
 * This behavior allows epsilon to be zero for exact comparison.
 */
inline bool approx(const float a, const float b, const float epsilon = FLT_EPSILON)
{
    return abs(a - b) <= max(abs(a), abs(b)) * epsilon;
}

/** Save asin calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
inline float asin(const float value) { return std::asin(clamp(value, -1, 1)); }

/** Save acos calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
inline float acos(const float value) { return std::acos(clamp(value, -1, 1)); }

/** Tests whether a given value is NAN. */
inline bool is_nan(const float value) { return std::isnan(value); }

/** Tests whether a given value is INFINITY. */
inline bool is_inf(const float value) { return std::isinf(value); }

/** Tests whether a given value is a valid float value (not NAN, not INFINITY). */
inline bool is_valid(const float value) { return !is_nan(value) && !is_inf(value); }

} // namespace notf
