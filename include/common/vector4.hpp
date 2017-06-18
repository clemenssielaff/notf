#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//*********************************************************************************************************************/

namespace detail {

/** 4-dimensional value with elements accessible through `x`, `y` `z` and `w` fields. */
template <typename VALUE_TYPE>
struct Value4 : public Value<VALUE_TYPE, 4> {

    /** Default (non-initializing) constructor so this struct remains a POD */
    Value4() = default;

    /** Element-wise constructor with 3 arguments and `w` set to 1. */
    template <typename A, typename B, typename C>
    Value4(A x, B y, C z)
        : array{{static_cast<VALUE_TYPE>(x), static_cast<VALUE_TYPE>(y),
                 static_cast<VALUE_TYPE>(z), 1}} {}

    /** Element-wise constructor. */
    template <typename A, typename B, typename C, typename D>
    Value4(A x, B y, C z, D w)
        : array{{static_cast<VALUE_TYPE>(x), static_cast<VALUE_TYPE>(y),
                 static_cast<VALUE_TYPE>(z), static_cast<VALUE_TYPE>(w)}} {}

    /** Value element data. */
    union {
        std::array<VALUE_TYPE, 4> array;
        struct {
            VALUE_TYPE x;
            VALUE_TYPE y;
            VALUE_TYPE z;
            VALUE_TYPE w;
        };
    };
};

} // namespace detail

//*********************************************************************************************************************/

/** 4-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector4 : public detail::Arithmetic<_RealVector4<Real>, detail::Value4<Real>> {

    // explitic forwards
    using super = detail::Arithmetic<_RealVector4<Real>, detail::Value4<Real>>;
    using super::x;
    using super::y;
    using super::z;
    using super::w;
    using value_t = typename super::value_t;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector4() = default;

    /** Perforect forwarding constructor. */
    template <typename... T>
    _RealVector4(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _RealVector4 x_axis() { return _RealVector4(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _RealVector4 y_axis() { return _RealVector4(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _RealVector4 z_axis() { return _RealVector4(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Checks whether this Vector3 is of unit magnitude. */
    bool is_unit() const { return abs(get_magnitude_sq() - 1) <= precision_high<value_t>(); }

    /** Returns the squared magnitude of this Vector3.
     * The squared magnitude is much cheaper to compute than the actual.
     */
    value_t get_magnitude_sq() const { return dot(*this); }

    /** Returns the magnitude of this Vector3. */
    value_t get_magnitude() const { return sqrt(dot(*this)); }

    /** Checks whether this Vector3 is parallel to other.
     * The zero Vector3 is parallel to everything.
     * @param other     Other Vector3.
     */
    bool is_parallel_to(const _RealVector4& other) const
    {
        return get_crossed(other).get_magnitude_sq() <= precision_high<value_t>();
    }

    /** Checks whether this Vector3 is orthogonal to other.
     * The zero Vector3 is orthogonal to everything.
     * @param other     Other Vector3.
     */
    bool is_orthogonal_to(const _RealVector4& other) const
    {
        return dot(other) <= precision_high<value_t>();
    }

    /** Calculates the smallest angle between two Vector3s.
     * Returns zero, if one or both of the input Vector3s are of zero magnitude.
     * @param other     Vector3 to test against.
     * @return Angle in positive radians.
     */
    value_t angle_to(const _RealVector4& other) const
    {
        const value_t mag_sq_product = get_magnitude_sq() * other.get_magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<value_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /** Tests if the other Vector3 is collinear (1), orthogonal(0), opposite (-1) or something in between.
     * Similar to `angle`, but saving a call to `acos`.
     * Returns zero, if one or both of the input Vector3s are of zero magnitude.
     * @param other     Vector3 to test against.
     */
    Real direction_to(const _RealVector4& other) const
    {
        const value_t mag_sq_product = get_magnitude_sq() * other.get_magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<value_t>()) {
            return clamp(dot(other), -1., 1.); // both are unit
        }
        return clamp(this->dot(other) / sqrt(mag_sq_product), -1., 1.);
    }

    /** Modification **************************************************************************************************/

    /** The sum of this value with another one. */
    _RealVector4 operator+(const _RealVector4& other) const { return super::operator+(other); }

    /** In-place addition of another value. */
    _RealVector4& operator+=(const _RealVector4& other) { return super::operator+=(other); }

    /** The difference between this value and another one. */
    _RealVector4 operator-(const _RealVector4& other) const { return super::operator-(other); }

    /** In-place subtraction of another value. */
    _RealVector4& operator-=(const _RealVector4& other) { return super::operator-=(other); }

    /** Component-wise multiplication of this value with another. */
    _RealVector4 operator*(const _RealVector4& other) const { return super::operator*(other); }

    /** In-place component-wise multiplication with another value. */
    _RealVector4& operator*=(const _RealVector4& other) { return super::operator*=(other); }

    /** Multiplication of this value with a scalar. */
    _RealVector4 operator*(const value_t factor) const { return super::operator*(factor); }

    /** In-place multiplication with a scalar. */
    _RealVector4& operator*=(const value_t factor) { return super::operator*=(factor); }

    /** Component-wise division of this value by another. */
    _RealVector4 operator/(const _RealVector4& other) const { return super::operator/(other); }

    /** In-place component-wise division by another value. */
    _RealVector4& operator/=(const _RealVector4& other) { return super::operator/=(other); }

    /** Division of this value by a scalar. */
    _RealVector4 operator/(const value_t divisor) const { return super::operator/(divisor); }

    /** In-place division by a scalar. */
    _RealVector4& operator/=(const value_t divisor) { return super::operator/=(divisor); }

    /** Returns the dot product of this Vector3 and another.
     * Allows calculation of the magnitude of one vector in the direction of another.
     * Can be used to determine in which general direction a Vector3 is positioned
     * in relation to another one.
     * @param other     Other Vector3.
     */
    value_t dot(const _RealVector4& other) const { return (x * other.x) + (y * other.y) + (z * other.z); }

    /** Vector3 cross product.
     * The cross product is a Vector3 perpendicular to this one and other.
     * The magnitude of the cross Vector3 is twice the area of the triangle
     * defined by the two input Vector3s.
     * @param other     Other Vector3.
     */
    _RealVector4 get_crossed(const _RealVector4& other) const
    {
        return _RealVector4(
            (y * other.z) - (z * other.y),
            (z * other.x) - (x * other.z),
            (x * other.y) - (y * other.x));
    }

    /** In-place Vector3 cross product.
     * @param other     Other Vector3.
     */
    _RealVector4& cross(const _RealVector4& other)
    {
        const value_t tx = (y * other.z) - (z * other.y);
        const value_t ty = (z * other.x) - (x * other.z);
        const value_t tz = (x * other.y) - (y * other.x);
        x                = tx;
        y                = ty;
        z                = tz;
        return *this;
    }

    /** Returns a normalized copy of this Vector3. */
    _RealVector4 get_normalized() const
    {
        const value_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<value_t>()) {
            return _RealVector4(*this); // is unit
        }
        if (abs(mag_sq) <= precision_high<value_t>()) {
            return _RealVector4(); // is zero
        }
        return *this * (1 / sqrt(mag_sq));
    }

    /** In-place normalization of this Vector3. */
    _RealVector4& normalize()
    {
        const value_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<value_t>()) {
            return *this; // is unit
        }
        if (abs(mag_sq) <= precision_high<value_t>()) {
            super::set_zero();
            return *this; // is zero
        }
        return *this *= (1 / sqrt(mag_sq));
    }

    /** Creates a projection of this Vector3 onto an infinite line whose direction is specified by other.
     * If the other Vector3 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector4 get_projected_on(const _RealVector4& other) { return other * dot(other); }

    /** Projects this Vector3 onto an infinite line whose direction is specified by other.
     * If the other Vector3 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector4 project_on(const _RealVector4& other)
    {
        *this = other * dot(other);
        return *this;
    }
};

using Vector4f = _RealVector4<float>;
using Vector4d = _RealVector4<double>;

// Free Functions *****************************************************************************************************/

/** Spherical linear interpolation between two Vector3s.
 * Travels the torque-minimal path at a constant velocity.
 * Taken from http://bulletphysics.org/Bullet/BulletFull/neon_2vec__aos_8h_source.html .
 * @param from      Left Vector, active at fade <= 0.
 * @param to        Right Vector, active at fade >= 1.
 * @param blend     Blend value, clamped to [0, 1].
 * @return          Interpolated Vector3.
 */
template <typename Real>
_RealVector4<Real> slerp(const _RealVector4<Real>& from, const _RealVector4<Real>& to, Real blend)
{
    blend = clamp(blend, 0., 1.);

    const Real cos_angle = from.dot(to);
    Real scale_0, scale_1;
    // use linear interpolation if the angle is too small
    if (cos_angle >= 1 - precision_low<Real>()) {
        scale_0 = (1.0f - blend);
        scale_1 = blend;
    }
    // otherwise use spherical interpolation
    else {
        const Real angle           = acos(cos_angle);
        const Real recip_sin_angle = (1.0f / sin(angle));
        scale_0                    = (sin(((1.0f - blend) * angle)) * recip_sin_angle);
        scale_1                    = (sin((blend * angle)) * recip_sin_angle);
    }
    return (from * scale_0) + (to * scale_1);
}

/** Prints the contents of a Vector3 into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param vec  Vector3 to print.
 * @return Output stream for further output.
 */
template <typename Real>
std::ostream& operator<<(std::ostream& out, const notf::_RealVector4<Real>& vec);

} // namespace notf

namespace std {

// std::hash **********************************************************************************************************/

/** std::hash _RealVector4 for notf::_RealVector3. */
template <typename Real>
struct hash<notf::_RealVector4<Real>> {
    size_t operator()(const notf::_RealVector4<Real>& vector) const { return vector.hash(); }
};

} // namespace std

// SIMD specializations ***********************************************************************************************/

#ifndef NOTF_NO_SIMD
namespace notf {

#include <emmintrin.h>

template <>
inline Vector4f Vector4f::operator+(const Vector4f& other) const
{
    Vector4f result;
    _mm_store_ps(result.as_ptr(), _mm_add_ps(_mm_load_ps(this->as_ptr()), _mm_load_ps(other.as_ptr())));
    return result;
}

template <>
inline Vector4f& Vector4f::operator+=(const Vector4f& other)
{
    _mm_store_ps(this->as_ptr(), _mm_add_ps(_mm_load_ps(this->as_ptr()), _mm_load_ps(other.as_ptr())));
    return *this;
}

} // namespace notf
#endif
