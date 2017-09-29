#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//*********************************************************************************************************************/

/** 3-dimensional mathematical vector containing real numbers.
 * While 3D vectors are useful for representing 3D data, use the 4D vector type for calculation and 3d vectors only for
 * storage.
 * Operations on 4D vectors take advantage of SIMD optimization and even in the general case (should not be) slower than
 * the equivalent operations on 3D vector types.
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector3 : public detail::Arithmetic<_RealVector3<Real>, Real, 3> {

    /* Types **********************************************************************************************************/

    using super   = detail::Arithmetic<_RealVector3<Real>, Real, 3>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector3() = default;

    /** Element-wise constructor. */
    template <typename A, typename B, typename C>
    _RealVector3(A x, B y, C z)
        : super{static_cast<value_t>(x), static_cast<value_t>(y), static_cast<value_t>(z)} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _RealVector3 x_axis() { return _RealVector3(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _RealVector3 y_axis() { return _RealVector3(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _RealVector3 z_axis() { return _RealVector3(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Returns True, if other and self are approximately the same Vector3.
     * @param other     Vector3 to test against.
     * @param epsilon   Maximal allowed distance between the two Vectors.
     */
    bool is_approx(const _RealVector3& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /** Checks whether this vector is parallel to other.
     * The zero vector is parallel to everything.
     * @param other     Other Vector3.
     */
    bool is_parallel_to(const _RealVector3& other) const
    {
        return this->cross(other).magnitude_sq() <= precision_high<value_t>();
    }

    /** Checks whether this vector is orthogonal to other.
     * The zero vector is orthogonal to everything.
     * @param other     Other Vector3.
     */
    bool is_orthogonal_to(const _RealVector3& other) const
    {
        return this->dot(other) <= precision_high<value_t>();
    }

    /** Calculates the smallest angle between two vectors.
     * Returns zero, if one or both of the input vectors are of zero magnitude.
     * @param other     Vector3 to test against.
     * @return Angle in positive radians.
     */
    value_t angle_to(const _RealVector3& other) const
    {
        const value_t mag_sq_product = super::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<value_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /** Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
     * Similar to `angle`, but saving a call to `acos`.
     * Returns zero, if one or both of the input vectors are of zero magnitude.
     * @param other     Vector3 to test against.
     */
    Real direction_to(const _RealVector3& other) const
    {
        const value_t mag_sq_product = super::magnitude_sq() * other.magnitude_sq();
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

    /** Operations ****************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Read-write access to the third element in the vector. */
    value_t& z() { return data[2]; }

    /** Returns the dot product of this Vector3 and another.
     * Allows calculation of the magnitude of one vector in the direction of another.
     * Can be used to determine in which general direction a Vector3 is positioned
     * in relation to another one.
     * @param other     Other Vector3.
     */
    value_t dot(const _RealVector3& other) const { return (x() * other.x()) + (y() * other.y()) + (z() * other.z()); }

    /** Vector3 cross product.
     * The cross product is a Vector3 perpendicular to this one and other.
     * The magnitude of the cross Vector3 is twice the area of the triangle
     * defined by the two input Vector3s.
     * The cross product is only defined for 3-dimensional vectors, the `w` element of the result will always be 1.
     * @param other     Other Vector3.
     */
    _RealVector3 cross(const _RealVector3& other) const
    {
        return _RealVector3(
            (y() * other.z()) - (z() * other.y()),
            (z() * other.x()) - (x() * other.z()),
            (x() * other.y()) - (y() * other.x()));
    }

    /** Projects this Vector3 onto an infinite line whose direction is specified by other.
     * If the other Vector3 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector3 project_on(const _RealVector3& other) const { return other * dot(other); }
};

//*********************************************************************************************************************/

/** 3-dimensional mathematical vector containing integers. */
template <typename Integer, ENABLE_IF_INT(Integer)>
struct _IntVector3 : public detail::Arithmetic<_IntVector3<Integer>, Integer, 3> {

    // explitic forwards
    using super   = detail::Arithmetic<_IntVector3<Integer>, Integer, 3>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _IntVector3() = default;

    /** Perforect forwarding constructor. */
    template <typename... T>
    _IntVector3(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _IntVector3 x_axis() { return _IntVector3(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _IntVector3 y_axis() { return _IntVector3(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _IntVector3 z_axis() { return _IntVector3(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Read-only access to the third element in the vector. */
    const value_t& z() const { return data[2]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Read-write access to the second element in the vector. */
    value_t& z() { return data[2]; }
};

//*********************************************************************************************************************/

using Vector3f = _RealVector3<float>;
using Vector3d = _RealVector3<double>;
using Vector3i = _IntVector3<int>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of a Vector3 into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param vec  Vector3 to print.
 * @return Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Vector3f& vec);
std::ostream& operator<<(std::ostream& out, const Vector3d& vec);
std::ostream& operator<<(std::ostream& out, const Vector3i& vec);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_RealVector3. */
template <typename Real>
struct hash<notf::_RealVector3<Real>> {
    size_t operator()(const notf::_RealVector3<Real>& vector) const { return vector.hash(); }
};

/** std::hash specialization for notf::_IntVector3. */
template <typename Integer>
struct hash<notf::_IntVector3<Integer>> {
    size_t operator()(const notf::_IntVector3<Integer>& vector) const { return vector.hash(); }
};

} // namespace std
