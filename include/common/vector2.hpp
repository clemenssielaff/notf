#pragma once

#include "common/arithmetic.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// 2-dimensional mathematical Vector containing real numbers.
template<typename REAL>
struct RealVector2 : public detail::Arithmetic<RealVector2<REAL>, REAL, 2> {

    /// Element type.
    using element_t = REAL;

    /// Arithmetic base type.
    using super_t = detail::Arithmetic<RealVector2<element_t>, element_t, 2>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    RealVector2() = default;

    /// Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    template<typename X, typename Y = element_t>
    RealVector2(const X x, const Y y = 0) : super_t{static_cast<element_t>(x), static_cast<element_t>(y)} {}

    /// Unit vector along the X-axis.
    static RealVector2 x_axis() { return RealVector2(1, 0); }

    /// Unit vector along the Y-axis.
    static RealVector2 y_axis() { return RealVector2(0, 1); }

    /// Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// Swizzles.
    RealVector2 xy() const { return { data[0], data[1] }; }
    RealVector2 yx() const { return { data[1], data[0] }; }

    /// Returns True, if other and self are approximately the same vector.
    /// @note Vectors use distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed distance between the two Vectors.
    bool is_approx(const RealVector2& other, const element_t epsilon = precision_high<element_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /// Checks whether this vector's direction is parallel to the other.
    /// @note The zero vector is parallel to every other vector.
    /// @param other    Vector to test against.
    bool is_parallel_to(const RealVector2& other) const
    {
        if (is_vertical()) {
            return abs(other.x()) <= precision_high<element_t>(); // both vertical
        }
        else if (is_horizontal()) {
            return abs(other.y()) <= precision_high<element_t>(); // both horizontal
        }
        return abs((x() / y()) - (other.x() / other.y())) <= precision_low<element_t>();
    }

    /// Returns the smallest angle (in radians) to the other vector.
    /// @note Always returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t angle_to(const RealVector2& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<element_t>()) {
            return acos(super_t::dot(other)); // both are unit
        }
        return acos(super_t::dot(other) / sqrt(mag_sq_product));
    }

    /// Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
    /// Similar to `angle`, but saving a call to `acos`.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t direction_to(const RealVector2& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<element_t>()) {
            return clamp(super_t::dot(other), -1, 1); // both are unit
        }
        return clamp(super_t::dot(other) / sqrt(mag_sq_product), -1, 1);
    }

    /// Tests if this vector is parallel to the Y-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_vertical() const { return abs(x()) <= precision_high<element_t>(); }

    /// Tests if this vector is parallel to the X-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_horizontal() const { return abs(y()) <= precision_high<element_t>(); }

    /// Returns the slope of this vector.
    /// @note If the vector is parallel to the y-axis, the slope is infinite.
    element_t slope() const
    {
        if (abs(x()) <= precision_high<element_t>()) {
            return INFINITY;
        }
        return y() / x();
    }

    /// Returns a vector orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
    /// @note The resulting vector is of the same magnitude as the original one.
    RealVector2 orthogonal() const { return RealVector2(-y(), x()); }

    /// Returns the cross product of this vector and another.
    /// As defined at http://mathworld.wolfram.com/CrossProduct.html
    /// Treats the 2D vectors like 3D vectors with z-components equal to zeros, takes their cross product, and returns
    /// the z-component of the result.
    /// @param other     Vector to the right.
    element_t cross(const RealVector2 other) const { return (x() * other.y()) - (y() * other.x()); }

    /// Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    RealVector2 project_on(const RealVector2& other) { return other * dot(other); }

    /// Returns a copy of this 2D Vector, rotated counter-clockwise by a given angle.
    /// @param angle    Angle in radians.
    RealVector2 rotate(const element_t angle) const
    {
        const element_t sin_angle = sin(angle);
        const element_t cos_angle = cos(angle);
        return RealVector2((x() * cos_angle) - (y() * sin_angle), (y() * cos_angle) + (x() * sin_angle));
    }

    /// Returns a copy this vector rotated around a pivot point by a given angle.
    /// @param angle    Angle in radians.
    RealVector2 rotate_around(const element_t angle, const RealVector2& pivot) const
    {
        return (*this - pivot).rotate(angle) += pivot;
    }
};

//====================================================================================================================//

/** 2-dimensional mathematical vector containing integers. */
template<typename INTEGER>
struct IntVector2 : public detail::Arithmetic<IntVector2<INTEGER>, INTEGER, 2> {

    /// Element type.
    using element_t = INTEGER;

    /// Arithmetic base type.
    using super_t = detail::Arithmetic<IntVector2<INTEGER>, INTEGER, 2>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    IntVector2() = default;

    /// Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    template<typename X, typename Y = element_t>
    IntVector2(const X x, const Y y = 0) : super_t{static_cast<element_t>(x), static_cast<element_t>(y)} {}

    /// Unit vector along the X-axis.
    static IntVector2 x_axis() { return IntVector2(1, 0); }

    /// Unit vector along the Y-axis.
    static IntVector2 y_axis() { return IntVector2(0, 1); }

    /// Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// Tests if this vector is parallel to the Y-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_vertical() const { return x() == 0; }

    /// Tests if this vector is parallel to the X-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_horizontal() const { return y() == 0; }

    /// Returns a vector orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
    /// @note The resulting vector is of the same magnitude as the original one.
    IntVector2 orthogonal() const { return IntVector2(-y(), x()); }
};

} // namespace detail

//====================================================================================================================//

using Vector2f = detail::RealVector2<float>;
using Vector2d = detail::RealVector2<double>;
using Vector2h = detail::RealVector2<half>;
using Vector2i = detail::IntVector2<int>;

//====================================================================================================================//

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector2f& vec);
std::ostream& operator<<(std::ostream& out, const Vector2d& vec);
std::ostream& operator<<(std::ostream& out, const Vector2h& vec);
std::ostream& operator<<(std::ostream& out, const Vector2i& vec);

} // namespace notf

//== std::hash =======================================================================================================//

namespace std {

/// std::hash specialization for RealVector2.
template<typename Real>
struct hash<notf::detail::RealVector2<Real>> {
    size_t operator()(const notf::detail::RealVector2<Real>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR), vector.hash());
    }
};

/// std::hash specialization for IntVector2.
template<typename Integer>
struct hash<notf::detail::IntVector2<Integer>> {
    size_t operator()(const notf::detail::IntVector2<Integer>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR), vector.hash());
    }
};

} // namespace std
