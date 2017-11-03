#pragma once

#include "common/arithmetic.hpp"

namespace notf {

//====================================================================================================================//

/// @brief  3-dimensional mathematical vector containing real numbers.
template<typename REAL>
struct _RealVector3 : public detail::Arithmetic<_RealVector3<REAL>, REAL, 3> {

    /// @brief Element type.
    using element_t = REAL;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_RealVector3<REAL>, REAL, 3>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _RealVector3() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    _RealVector3(const element_t x, const element_t y = 0, const element_t z = 0) : super_t{x, y, z} {}

    /** Unit vector along the X-axis. */
    static _RealVector3 x_axis() { return _RealVector3(1, 0, 0); }

    /** Unit vector along the Y-axis. */
    static _RealVector3 y_axis() { return _RealVector3(0, 1, 0); }

    /** Unit vector along the Z-axis. */
    static _RealVector3 z_axis() { return _RealVector3(0, 0, 1); }

    /// @brief Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// @brief Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// @brief Read-write access to the third element in the vector.
    element_t& z() { return data[2]; }

    /// @brief Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// @brief Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// @brief Read-only access to the third element in the vector.
    const element_t& z() const { return data[2]; }

    /// @brief Returns True, if other and self are approximately the same vector.
    /// @note Vector3s use distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed squared distance between the two vectors.
    bool is_approx(const _RealVector3& other, const element_t epsilon = precision_high<element_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon;
    }

    /// @brief Checks whether this vector is parallel to other.
    /// @note The zero vector is parallel to everything.
    /// @param other    Vector to test against.
    bool is_parallel_to(const _RealVector3& other) const
    {
        return cross(other).magnitude_sq() <= precision_high<element_t>();
    }

    /// @brief Calculates the smallest angle between two vectors.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    /// @return Angle in positive radians.
    element_t angle_to(const _RealVector3& other) const
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

    /// @brief Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
    /// Similar to `angle`, but saving a call to `acos`.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t direction_to(const _RealVector3& other) const
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

    /// @brief Vector cross product.
    /// The cross product is a vector perpendicular to this one and other.
    /// The magnitude of the cross vector is twice the area of the triangle defined by the two input Vector3s.
    /// @param other     Other vector.
    _RealVector3 cross(const _RealVector3& other) const
    {
        return _RealVector3((y() * other.z()) - (z() * other.y()), (z() * other.x()) - (x() * other.z()),
                            (x() * other.y()) - (y() * other.x()));
    }

    /// @brief Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    _RealVector3 project_on(const _RealVector3& other) const { return other * dot(other); }
};

//====================================================================================================================//

/** 3-dimensional mathematical vector containing integers. */
template<typename INTEGER>
struct _IntVector3 : public detail::Arithmetic<_IntVector3<INTEGER>, INTEGER, 3> {

    /// @brief Element type.
    using element_t = INTEGER;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_IntVector3<INTEGER>, INTEGER, 3>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _IntVector3() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    _IntVector3(const element_t x, const element_t y = 0, const element_t z = 0) : super_t{x, y, z} {}

    /** Unit vector along the X-axis. */
    static _IntVector3 x_axis() { return _IntVector3(1, 0, 0); }

    /** Unit vector along the Y-axis. */
    static _IntVector3 y_axis() { return _IntVector3(0, 1, 0); }

    /** Unit vector along the Z-axis. */
    static _IntVector3 z_axis() { return _IntVector3(0, 0, 1); }

    /// @brief Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// @brief Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// @brief Read-write access to the third element in the vector.
    element_t& z() { return data[2]; }

    /// @brief Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// @brief Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// @brief Read-only access to the third element in the vector.
    const element_t& z() const { return data[2]; }
};

//====================================================================================================================//

using Vector3f = _RealVector3<float>;
using Vector3d = _RealVector3<double>;
using Vector3h = _RealVector3<half>;
using Vector3i = _IntVector3<int>;

//====================================================================================================================//

/// @brief Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector3f& vec);
std::ostream& operator<<(std::ostream& out, const Vector3d& vec);
std::ostream& operator<<(std::ostream& out, const Vector3h& vec);
std::ostream& operator<<(std::ostream& out, const Vector3i& vec);

//====================================================================================================================//

/// @brief Spherical linear interpolation between two vectors.
/// Travels the torque-minimal path at a constant velocity.
/// Taken from http://bulletphysics.org/Bullet/BulletFull/neon_2vec__aos_8h_source.html .
/// @param from      Left Vector, active at fade <= 0.
/// @param to        Right Vector, active at fade >= 1.
/// @param blend     Blend value, clamped to [0, 1].
/// @return          Interpolated vector.
template<typename element_t>
_RealVector3<element_t> slerp(const _RealVector3<element_t>& from, const _RealVector3<element_t>& to, element_t blend)
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

//====================================================================================================================//

namespace std {

/** std::hash specialization for notf::_RealVector3. */
template<typename Real>
struct hash<notf::_RealVector3<Real>> {
    size_t operator()(const notf::_RealVector3<Real>& vector) const { return vector.hash(); }
};

/** std::hash specialization for notf::_IntVector3. */
template<typename Integer>
struct hash<notf::_IntVector3<Integer>> {
    size_t operator()(const notf::_IntVector3<Integer>& vector) const { return vector.hash(); }
};

} // namespace std
