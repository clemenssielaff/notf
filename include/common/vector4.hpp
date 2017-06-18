#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//*********************************************************************************************************************/

/** 4-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector4 : public detail::Arithmetic<_RealVector4<Real>, Real, 4> {

    // explitic forwards
    using super   = detail::Arithmetic<_RealVector4<Real>, Real, 4>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector4() = default;

    /** Element-wise constructor with 3 arguments and `w` set to 1. */
    template <typename A, typename B, typename C>
    _RealVector4(A x, B y, C z)
        : super({static_cast<value_t>(x), static_cast<value_t>(y),
                 static_cast<value_t>(z), static_cast<value_t>(1)}) {}

    /** Element-wise constructor. */
    template <typename A, typename B, typename C, typename D>
    _RealVector4(A x, B y, C z, D w)
        : super({static_cast<value_t>(x), static_cast<value_t>(y),
                 static_cast<value_t>(z), static_cast<value_t>(w)}) {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _RealVector4 x_axis() { return _RealVector4(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _RealVector4 y_axis() { return _RealVector4(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _RealVector4 z_axis() { return _RealVector4(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

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
        const value_t mag_sq_product = super::get_magnitude_sq() * other.get_magnitude_sq();
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
        const value_t mag_sq_product = super::get_magnitude_sq() * other.get_magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<value_t>()) {
            return clamp(dot(other), -1., 1.); // both are unit
        }
        return clamp(this->dot(other) / sqrt(mag_sq_product), -1., 1.);
    }

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Read-only access to the third element in the vector. */
    const value_t& z() const { return data[2]; }

    /** Read-only access to the fourth element in the vector. */
    const value_t& w() const { return data[3]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Read-write access to the third element in the vector. */
    value_t& z() { return data[2]; }

    /** Read-write access to the fourth element in the vector. */
    value_t& w() { return data[3]; }

    /** Returns the dot product of this Vector3 and another.
     * Allows calculation of the magnitude of one vector in the direction of another.
     * Can be used to determine in which general direction a Vector3 is positioned
     * in relation to another one.
     * @param other     Other Vector3.
     */
    value_t dot(const _RealVector4& other) const { return (x() * other.x()) + (y() * other.y()) + (z() * other.z()); }

    /** Vector3 cross product.
     * The cross product is a Vector3 perpendicular to this one and other.
     * The magnitude of the cross Vector3 is twice the area of the triangle
     * defined by the two input Vector3s.
     * @param other     Other Vector3.
     */
    _RealVector4 get_crossed(const _RealVector4& other) const
    {
        return _RealVector4(
            (y() * other.z()) - (z() * other.y()),
            (z() * other.x()) - (x() * other.z()),
            (x() * other.y()) - (y() * other.x()));
    }

    /** In-place Vector3 cross product.
     * @param other     Other Vector3.
     */
    _RealVector4& cross(const _RealVector4& other)
    {
        const value_t tx = (y() * other.z()) - (z() * other.y());
        const value_t ty = (z() * other.x()) - (x() * other.z());
        const value_t tz = (x() * other.y()) - (y() * other.x());
        x()              = tx;
        y()              = ty;
        z()              = tz;
        return *this;
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
