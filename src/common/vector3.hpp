#pragma once

#include "common/arithmetic.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

///  3-dimensional mathematical vector containing real numbers.
template<typename REAL>
struct RealVector3 : public detail::Arithmetic<RealVector3<REAL>, REAL, 3> {

    /// Element type.
    using element_t = REAL;

    /// Arithmetic base type.
    using super_t = detail::Arithmetic<RealVector3<REAL>, REAL, 3>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    RealVector3() = default;

    /// Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    template<typename X, typename Y = element_t, typename Z = element_t>
    RealVector3(const X x, const Y y = 0, const Z z = 0)
        : super_t{static_cast<element_t>(x), static_cast<element_t>(y), static_cast<element_t>(z)}
    {}

    /** Unit vector along the X-axis. */
    static RealVector3 x_axis() { return RealVector3(1, 0, 0); }

    /** Unit vector along the Y-axis. */
    static RealVector3 y_axis() { return RealVector3(0, 1, 0); }

    /** Unit vector along the Z-axis. */
    static RealVector3 z_axis() { return RealVector3(0, 0, 1); }

    /// Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// Read-write access to the third element in the vector.
    element_t& z() { return data[2]; }

    /// Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// Read-only access to the third element in the vector.
    const element_t& z() const { return data[2]; }

    /// Swizzles.
    RealVector3 xyz() const { return {data[0], data[1], data[2]}; }
    RealVector3 xzy() const { return {data[0], data[2], data[1]}; }
    RealVector3 yxz() const { return {data[1], data[0], data[2]}; }
    RealVector3 yzx() const { return {data[1], data[2], data[0]}; }
    RealVector3 zxy() const { return {data[2], data[0], data[1]}; }
    RealVector3 zyx() const { return {data[2], data[1], data[0]}; }

    /// Returns True, if other and self are approximately the same vector.
    /// @note Vector3s use distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed squared distance between the two vectors.
    bool is_approx(const RealVector3& other, const element_t epsilon = precision_high<element_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon;
    }

    /// Checks whether this vector is parallel to other.
    /// @note The zero vector is parallel to everything.
    /// @param other    Vector to test against.
    bool is_parallel_to(const RealVector3& other) const
    {
        return cross(other).magnitude_sq() <= precision_high<element_t>();
    }

    /// Checks whether this vector is orthogonal to other.
    /// @note The zero vector is orthogonal to everything.
    /// @param other    Vector to test against.
    bool is_orthogonal_to(const RealVector3& other) const { return direction_to(other) < precision_high<element_t>(); }

    /// Calculates the smallest angle between two vectors.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    /// @return Angle in positive radians.
    element_t angle_to(const RealVector3& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<element_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /// Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
    /// Similar to `angle`, but saving a call to `acos`.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t direction_to(const RealVector3& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<element_t>()) {
            return clamp(dot(other), -1., 1.); // both are unit
        }
        return clamp(this->dot(other) / sqrt(mag_sq_product), -1., 1.);
    }

    /// Vector cross product.
    /// The cross product is a vector perpendicular to this one and other.
    /// The magnitude of the cross vector is twice the area of the triangle defined by the two input Vector3s.
    /// @param other     Other vector.
    RealVector3 cross(const RealVector3& other) const
    {
        return RealVector3((y() * other.z()) - (z() * other.y()), (z() * other.x()) - (x() * other.z()),
                           (x() * other.y()) - (y() * other.x()));
    }

    /// Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    RealVector3 project_on(const RealVector3& other) const { return other * dot(other); }
};

//====================================================================================================================//

/** 3-dimensional mathematical vector containing integers. */
template<typename INTEGER>
struct IntVector3 : public detail::Arithmetic<IntVector3<INTEGER>, INTEGER, 3> {

    /// Element type.
    using element_t = INTEGER;

    /// Arithmetic base type.
    using super_t = detail::Arithmetic<IntVector3<INTEGER>, INTEGER, 3>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    IntVector3() = default;

    /// Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    template<typename X, typename Y = element_t, typename Z = element_t>
    IntVector3(const X x, const Y y = 0, const Z z = 0)
        : super_t{static_cast<element_t>(x), static_cast<element_t>(y), static_cast<element_t>(z)}
    {}

    /// Unit vector along the X-axis.
    static IntVector3 x_axis() { return IntVector3(1, 0, 0); }

    /// Unit vector along the Y-axis.
    static IntVector3 y_axis() { return IntVector3(0, 1, 0); }

    /// Unit vector along the Z-axis.
    static IntVector3 z_axis() { return IntVector3(0, 0, 1); }

    /// Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// Read-write access to the third element in the vector.
    element_t& z() { return data[2]; }

    /// Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// Read-only access to the third element in the vector.
    const element_t& z() const { return data[2]; }
};

} // namespace detail

//====================================================================================================================//

using Vector3f = detail::RealVector3<float>;
using Vector3d = detail::RealVector3<double>;
using Vector3h = detail::RealVector3<half>;
using Vector3i = detail::IntVector3<int>;

//====================================================================================================================//

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector3f& vec);
std::ostream& operator<<(std::ostream& out, const Vector3d& vec);
std::ostream& operator<<(std::ostream& out, const Vector3h& vec);
std::ostream& operator<<(std::ostream& out, const Vector3i& vec);

//====================================================================================================================//

/// Spherical linear interpolation between two vectors.
/// Travels the torque-minimal path at a constant velocity.
/// Taken from http://bulletphysics.org/Bullet/BulletFull/neon_2vec__aos_8h_source.html .
/// @param from      Left Vector, active at fade <= 0.
/// @param to        Right Vector, active at fade >= 1.
/// @param blend     Blend value, clamped to [0, 1].
/// @return          Interpolated vector.
template<typename element_t>
detail::RealVector3<element_t>
slerp(const detail::RealVector3<element_t>& from, const detail::RealVector3<element_t>& to, element_t blend)
{
    blend = clamp(blend, 0., 1.);

    const element_t cos_angle = from.dot(to);
    element_t scale_0, scale_1;
    // use linear interpolation if the angle is too small
    if (cos_angle >= 1 - precision_low<element_t>()) {
        scale_0 = (1.0f - blend);
        scale_1 = blend;
    }
    // otherwise use spherical interpolation
    else {
        const element_t angle           = acos(cos_angle);
        const element_t recip_sin_angle = 1 / sin(angle);

        scale_0 = (sin(((1.0f - blend) * angle)) * recip_sin_angle);
        scale_1 = (sin((blend * angle)) * recip_sin_angle);
    }
    return (from * scale_0) + (to * scale_1);
}

} // namespace notf

//== std::hash =======================================================================================================//

namespace std {

/// std::hash specialization for RealVector3.
template<typename REAL>
struct hash<notf::detail::RealVector3<REAL>> {
    size_t operator()(const notf::detail::RealVector3<REAL>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR), vector.hash());
    }
};

/// std::hash specialization for IntVector3.
template<typename INTEGER>
struct hash<notf::detail::IntVector3<INTEGER>> {
    size_t operator()(const notf::detail::IntVector3<INTEGER>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR), vector.hash());
    }
};

} // namespace std
