#pragma once

#include <cfloat>
#include <cmath>
#include <functional>
#include <limits>

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

/** Tests whether a given value is NAN. */
inline bool is_nan(const float value) { return std::isnan(value); }

/** Tests whether a given value is INFINITY. */
inline bool is_inf(const float value) { return std::isinf(value); }

/** Tests whether a given value is a valid float value (not NAN, not INFINITY). */
inline bool is_real(const float value) { return !is_nan(value) && !is_inf(value); }

/** Tests, if a value is positive or negative.
 * @return  -1 if the value is negative, 1 if it is zero or above.
 */
inline float sign(const float value) { return std::signbit(value) ? -1 : 1; }

/** Clamps an input value to a given range. */
inline float clamp(const float value, const float min, const float max)
{
    return value < min ? min : (value > max ? max : value);
}

/** Save asin calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
inline float asin(const float value) { return std::asin(clamp(value, -1, 1)); }

/** Save acos calculation.
 * @param value     Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
 */
inline float acos(const float value) { return std::acos(clamp(value, -1, 1)); }

/** Test if two Reals are approximately the same value.
 * Returns true also if the difference is exactly epsilon.
 * This behavior allows epsilon to be zero for exact comparison.
 *
 * Example:
 *
 * ```
 * bool is_approx = 1.1234 == approx(1.2345, 0.1);
 * ```
 *
 * Comparison function from:
 * From https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 *
 * approx-class idea from catch:
 * https://github.com/philsquared/Catch/blob/master/include/catch.hpp
 */
class approx {
public:
    explicit approx(float value, float epsilon = std::numeric_limits<float>::epsilon() * 100)
        : m_value(value)
        , m_epsilon(epsilon)
    {
    }

    friend bool operator==(float lhs, const approx& rhs)
    {
        if (!is_real(lhs) || !is_real(rhs.m_value)) {
            return false;
        }
        return abs(lhs - rhs.m_value) <= max(abs(lhs), abs(rhs.m_value)) * rhs.m_epsilon;
    }

    friend bool operator==(const approx& lhs, float rhs) { return operator==(rhs, lhs); }

    friend bool operator!=(float lhs, const approx& rhs) { return !operator==(lhs, rhs); }

    friend bool operator!=(const approx& lhs, float rhs) { return !operator==(rhs, lhs); }

private:
    float m_value;
    float m_epsilon;
};

} // namespace notf
