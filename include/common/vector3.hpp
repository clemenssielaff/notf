#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "utils/sfinae.hpp"

namespace notf {
//*********************************************************************************************************************/

/** 3-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector3 {
    /* Types **********************************************************************************************************/

    using Value_t = Real;

    /* Fields *********************************************************************************************************/

    /** X-coordinate. */
    Value_t x;

    /** Y-coordinate */
    Value_t y;

    /** Z-coordinate */
    Value_t z;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector3() = default;

    /** Creates a Vector3 with the given components.
     * @param x     X-coordinate.
     * @param y     Y-coordinate.
     */
    _RealVector3(const Value_t x, const Value_t y, const Value_t z)
        : x(x), y(y), z(z) {}

    /* Static Constructors ********************************************************************************************/

    /** A zero Vector3. */
    static _RealVector3 zero() { return _RealVector3(0, 0, 0); }

    /** Constructs a Vector3 with both coordinates set to the given value. */
    static _RealVector3 fill(const Value_t value) { return _RealVector3(value, value, value); }

    /** Unit Vector3 along the X-axis. */
    static _RealVector3 x_axis() { return _RealVector3(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _RealVector3 y_axis() { return _RealVector3(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _RealVector3 z_axis() { return _RealVector3(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Checks, if this Vector3 contains only real, finite values (no INFINITY or NAN). */
    bool is_real() const { return is_real(x) && is_real(y) && is_real(z); }

    /** Returns true if all coordinates are (approximately) zero.
     * @param epsilon   Largest difference that is still considered to be zero.
     */
    bool is_zero(const Value_t epsilon = precision_high<Value_t>()) const
    {
        return abs(x) <= epsilon && abs(y) <= epsilon && abs(z) <= epsilon;
    }

    /** Checks, if any component of this Vector3 is a zero. */
    bool contains_zero() const
    {
        return abs(x) <= precision_high<Value_t>() || abs(y) <= precision_high<Value_t>() || abs(z) <= precision_high<Value_t>();
    }

    /** Checks whether this Vector3 is of unit magnitude. */
    bool is_unit() const { return abs(magnitude_sq() - 1) <= precision_high<Value_t>(); }

    /** Returns True, if other and self are approximately the same Vector3.
     * @param other     Vector3 to test against.
     * @param epsilon   Maximal allowed distance between the two Vectors.
     */
    bool is_approx(const _RealVector3& other, const Value_t epsilon = precision_high<Value_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /** Returns the squared magnitude of this Vector3.
     * The squared magnitude is much cheaper to compute than the actual.
     */
    Value_t magnitude_sq() const { return dot(*this); }

    /** Returns the magnitude of this Vector3. */
    Value_t magnitude() const { return sqrt(dot(*this)); }

    /** Checks whether this Vector3 is parallel to other.
     * The zero Vector3 is parallel to everything.
     * @param other     Other Vector3.
     */
    bool is_parallel_to(const _RealVector3& other) const
    {
        return crossed(other).magnitude_sq() <= precision_high<Value_t>();
    }

    /** Checks whether this Vector3 is orthogonal to other.
     * The zero Vector3 is orthogonal to everything.
     * @param other     Other Vector3.
     */
    bool is_orthogonal_to(const _RealVector3& other) const
    {
        return dot(other) <= precision_high<Value_t>();
    }

    /** Calculates the smallest angle between two Vector3s.
     * Returns zero, if one or both of the input Vector3s are of zero magnitude.
     * @param other     Vector3 to test against.
     * @return Angle in positive radians.
     */
    Value_t angle_to(const _RealVector3& other) const
    {
        const Value_t mag_sq_product = magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<Value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<Value_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /** Tests if the other Vector3 is collinear (1), orthogonal(0), opposite (-1) or something in between.
     * Similar to `angle`, but saving a call to `acos`.
     * Returns zero, if one or both of the input Vector3s are of zero magnitude.
     * @param other     Vector3 to test against.
     */
    Real direction_to(const _RealVector3& other) const
    {
        const Value_t mag_sq_product = magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<Value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<Value_t>()) {
            return clamp(dot(other), -1., 1.); // both are unit
        }
        return clamp(this->dot(other) / sqrt(mag_sq_product), -1., 1.);
    }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Vector3s are equal. */
    bool operator==(const _RealVector3& other) const { return (*this - other).is_zero(); }

    /** Tests whether two Vector3s not are equal. */
    bool operator!=(const _RealVector3& other) const { return !(*this == other); }

    /** Adding another Vector3 moves this Vector3 relative to its previous position. */
    _RealVector3 operator+(const _RealVector3& other) const { return _RealVector3(x + other.x, y + other.y, z + other.z); }

    /** In-place addition of another Vector3. */
    _RealVector3& operator+=(const _RealVector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    /** Subtracting another Vector3 places this Vector3's start point at the other's end. */
    _RealVector3 operator-(const _RealVector3& other) const { return _RealVector3(x - other.x, y - other.y, z - other.z); }

    /** In-place subtraction of another Vector3. */
    _RealVector3& operator-=(const _RealVector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    /** Component-wise multiplication of a Vector3 with another Vector3. */
    _RealVector3 operator*(const _RealVector3& other) const { return _RealVector3(x * other.x, y * other.y, z * other.z); }

    /** In-place component-wise multiplication of a Vector3 with another Vector3. */
    _RealVector3& operator*=(const _RealVector3& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    /** Component-wise division of a Vector3 by another Vector3. */
    _RealVector3 operator/(const _RealVector3& other) const
    {
        return {save_div(x, other.x), save_div(y, other.y), save_div(z, other.z)};
    }

    /** In-place component-wise division of a Vector3 by another Vector3. */
    _RealVector3& operator/=(const _RealVector3& other)
    {
        x = save_div(x, other.x);
        y = save_div(y, other.y);
        z = save_div(z, other.z);
        return *this;
    }

    /** Multiplication with a scalar scales this Vector3's length. */
    _RealVector3 operator*(const Value_t factor) const { return _RealVector3(x * factor, y * factor, z * factor); }

    /** In-place multiplication with a scalar. */
    _RealVector3& operator*=(const Value_t factor)
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    /** Division by a scalar inversely scales this Vector3's length.
     * If you know that divisor cannot be zero, calling `vector *= 1/divisor` will save you a 'division-by-zero' test.
     */
    _RealVector3 operator/(const Value_t divisor) const
    {
        if (divisor == 0) {
            throw division_by_zero();
        }
        return _RealVector3(x / divisor, y / divisor, z / divisor);
    }

    /** In-place division by a scalar value.
     * If you know that divisor cannot be zero, calling `vector *= 1/divisor` will save you a 'division-by-zero' test.
     */
    _RealVector3& operator/=(const Value_t divisor)
    {
        if (divisor == 0) {
            throw division_by_zero();
        }
        x /= divisor;
        y /= divisor;
        z /= divisor;
        return *this;
    }

    /** Returns an inverted copy of this Vector3. */
    _RealVector3 operator-() const { return inverse(); }

    /** Read-only access to the Vector3's internal storage. */
    template <typename Index, ENABLE_IF_INT(Index)>
    const float& operator[](const Index index) const
    {
        if (0 > index || index > 2) {
            throw std::range_error("Index requested via Vector3 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Read/write access to the Vector3's internal storage. */
    template <typename Index, ENABLE_IF_INT(Index)>
    float& operator[](const Index index)
    {
        if (0 > index || index > 2) {
            throw std::range_error("Index requested via Vector3 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Read-only pointer to the Vector3's internal storage. */
    const Value_t* as_ptr() const { return &x; }

    /** Read-write pointer to the Vector3's internal storage. */
    Value_t* as_ptr() { return &x; }

    /** Modifiers *****************************************************************************************************/

    /** Sets all components to zero. */
    _RealVector3& set_zero()
    {
        x = y = z = 0;
        return *this;
    }

    /** Returns an inverted copy of this Vector3. */
    _RealVector3 inverse() const { return _RealVector3(-x, -y, -z); }

    /** Inverts this Vector3 in-place. */
    _RealVector3& invert()
    {
        x = -x;
        y = -y;
        z = -z;
        return *this;
    }

    /** Returns the dot product of this Vector3 and another.
     * Allows calculation of the magnitude of one vector in the direction of another.
     * Can be used to determine in which general direction a Vector3 is positioned
     * in relation to another one.
     * @param other     Other Vector3.
     */
    Value_t dot(const _RealVector3& other) const { return (x * other.x) + (y * other.y) + (z * other.z); }

    /** Vector3 cross product.
     * The cross product is a Vector3 perpendicular to this one and other.
     * The magnitude of the cross Vector3 is twice the area of the triangle
     * defined by the two input Vector3s.
     * @param other     Other Vector3.
     */
    _RealVector3 crossed(const _RealVector3& other) const
    {
        return _RealVector3(
            (y * other.z) - (z * other.y),
            (z * other.x) - (x * other.z),
            (x * other.y) - (y * other.x));
    }

    /** In-place Vector3 cross product.
     * @param other     Other Vector3.
     */
    _RealVector3& cross(const _RealVector3& other)
    {
        const Value_t tx = (y * other.z) - (z * other.y);
        const Value_t ty = (z * other.x) - (x * other.z);
        const Value_t tz = (x * other.y) - (y * other.x);
        x                = tx;
        y                = ty;
        z                = tz;
        return *this;
    }

    /** Returns a normalized copy of this Vector3. */
    _RealVector3 normalized() const
    {
        const Value_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<Value_t>()) {
            return _RealVector3(*this); // is unit
        }
        if (abs(mag_sq) <= precision_high<Value_t>()) {
            return _RealVector3(); // is zero
        }
        return *this * (1 / sqrt(mag_sq));
    }

    /** In-place normalization of this Vector3. */
    _RealVector3& normalize()
    {
        const Value_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<Value_t>()) {
            return (*this); // is unit
        }
        if (abs(mag_sq) <= precision_high<Value_t>()) {
            return set_zero(); // is zero
        }
        return *this *= (1 / sqrt(mag_sq));
    }

    /** Creates a projection of this Vector3 onto an infinite line whose direction is specified by other.
     * If the other Vector3 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector3 projected_on(const _RealVector3& other) { return other * dot(other); }

    /** Projects this Vector3 onto an infinite line whose direction is specified by other.
     * If the other Vector3 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector3 project_on(const _RealVector3& other)
    {
        *this = other * dot(other);
        return *this;
    }
};

//*********************************************************************************************************************/

using Vector3f = _RealVector3<float>;
using Vector3d = _RealVector3<double>;

/* Free Functions *****************************************************************************************************/

/** Linear interpolation between two Vector3s.
 * @param from    Left Vector, full weight at blend <= 0.
 * @param to      Right Vector, full weight at blend >= 1.
 * @param blend   Blend value, clamped to range [0, 1].
 */
template <typename Real>
inline _RealVector3<Real> lerp(const _RealVector3<Real>& from, const _RealVector3<Real>& to, const Real blend)
{
    return ((to - from) *= clamp(blend, 0, 1)) += from;
}

/** Spherical linear interpolation between two Vector3s.
 * Travels the torque-minimal path at a constant velocity.
 * Taken from http://bulletphysics.org/Bullet/BulletFull/neon_2vec__aos_8h_source.html .
 * @param from      Left Vector, active at fade <= 0.
 * @param to        Right Vector, active at fade >= 1.
 * @param blend     Blend value, clamped to [0, 1].
 * @return          Interpolated Vector3.
 */
template <typename Real>
_RealVector3<Real> slerp(const _RealVector3<Real>& from, const _RealVector3<Real>& to, Real blend)
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
std::ostream& operator<<(std::ostream& out, const notf::_RealVector3<Real>& vec);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_RealVector3. */
template <typename Real>
struct hash<notf::_RealVector3<Real>> {
    size_t operator()(const notf::_RealVector3<Real>& vector) const { return notf::hash(vector.x, vector.y, vector.z); }
};

} // namespace std
