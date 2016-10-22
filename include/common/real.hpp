#pragma once

#include <cfloat>
#include <cmath>
#include <functional>

namespace notf {

#ifdef SIGNAL_DOUBLE_PRECISION
using Real = double;
#else
using Real = float;
#endif

///
/// \brief PI.
///
extern const Real HALF_PI;
extern const Real PI;
extern const Real TWO_PI;

using std::abs;
using std::max;
using std::min;
using std::sqrt;
using std::fmod;
using std::sin;
using std::cos;
using std::tan;
using std::atan2;
const Real bla  = INFINITY;

/// \brief Tests, if a value is positive or negative.
///
/// \param value    Value to test.
///
/// \return -1 if the value is negative, 1 if it is zero or above.
inline Real sign(const Real value) { return std::signbit(value) ? -1 : 1; }

/// \brief Clamps an input value to a given range.
///
/// \param value    Value to clamp.
/// \param min      Lower end of range (exclusive).
/// \param max      Upper end of range (inclusive).
///
/// \return New, clamped value.
inline Real clamp(const Real value, const Real min, const Real max)
{
    return value < min ? min : (value > max ? max : value);
}

/// \brief Test if two Reals are approximately the same value.
///
/// From https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
///
/// approx() returns true also if the difference is exactly epsilon.
/// This behavior allows epsilon to be zero for exact comparison.
///
/// \param a       First Real.
/// \param b       Second Real.
/// \param epsilon Maximal relative error.
///
/// \return True, if both inputs are approximately the same, False otherwise.
inline bool approx(const Real a, const Real b, const Real epsilon = FLT_EPSILON)
{
    return abs(a - b) <= max(abs(a), abs(b)) * epsilon;
}

/// \brief Save asin calculation
///
/// \param value   Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to asin.
///
/// \return The arc-sine of the input value.
inline Real asin(const Real value) { return std::asin(clamp(value, -1, 1)); }

/// \brief Save acos calculation
///
/// \param value   Input, is clamped to the range of [-1.0 ... 1.0], prior to the call to acos.
///
/// \return The arc-cosine of the input value.
inline Real acos(const Real value) { return std::acos(clamp(value, -1, 1)); }

/// \brief Calculates a hash value from a supplied Real.
///
/// \param value    Real to hash.
inline size_t hash(const Real value) { return std::hash<Real>()(value); }

/// \brief Tests whether a given value is NAN.
inline bool is_nan(const Real value) { return std::isnan(value); }

/// \brief Tests whether a given value is INFINITY.
inline bool is_inf(const Real value) { return std::isinf(value); }

/// \brief Tests whether a given value is a valid Real value (not NAN, not INFINITY).
inline bool is_valid(const Real value) { return !is_nan(value) && !is_inf(value); }

/// \brief Builds up a hash value from an existing hash AND the supplied Real.
///
/// Useful for building up hashes from several Reals, like in a Vector2 - for example.
///
/// \param value    Real to hash.
/// \param seed     Existing hash value to build up upon.
///
/// \return New hash value.
inline size_t hash(Real value, size_t seed)
{
    size_t result = seed;
    result ^= hash(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return result;
}

} // namespace notf
