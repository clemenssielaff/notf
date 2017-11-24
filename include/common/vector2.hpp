#pragma once

#include "common/arithmetic.hpp"

namespace notf {

//====================================================================================================================//

/// @brief 2-dimensional mathematical Vector containing real numbers.
template<typename REAL>
struct _RealVector2 : public detail::Arithmetic<_RealVector2<REAL>, REAL, 2> {

    /// @brief Element type.
    using element_t = REAL;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_RealVector2<element_t>, element_t, 2>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _RealVector2() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    _RealVector2(const element_t x, const element_t y = 0) : super_t{x, y} {}

    /// @brief Unit vector along the X-axis.
    static _RealVector2 x_axis() { return _RealVector2(1, 0); }

    /// @brief Unit vector along the Y-axis.
    static _RealVector2 y_axis() { return _RealVector2(0, 1); }

    /// @brief Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// @brief Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// @brief Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// @brief Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// @brief Returns True, if other and self are approximately the same vector.
    /// @note Vectors use distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed distance between the two Vectors.
    bool is_approx(const _RealVector2& other, const element_t epsilon = precision_high<element_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /// @brief Checks whether this vector's direction is parallel to the other.
    /// @note The zero vector is parallel to every other vector.
    /// @param other    Vector to test against.
    bool is_parallel_to(const _RealVector2& other) const
    {
        if (is_vertical()) {
            return abs(other.x()) <= precision_high<element_t>(); // both vertical
        }
        else if (is_horizontal()) {
            return abs(other.y()) <= precision_high<element_t>(); // both horizontal
        }
        return abs((x() / y()) - (other.x() / other.y())) <= precision_low<element_t>();
    }

    /// @brief Returns the smallest angle (in radians) to the other vector.
    /// @note Always returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t angle_to(const _RealVector2& other) const
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

    /// @brief Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
    /// Similar to `angle`, but saving a call to `acos`.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t direction_to(const _RealVector2& other) const
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

    /// @brief Tests if this vector is parallel to the Y-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_vertical() const { return abs(x()) <= precision_high<element_t>(); }

    /// @brief Tests if this vector is parallel to the X-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_horizontal() const { return abs(y()) <= precision_high<element_t>(); }

    /// @brief Returns the slope of this vector.
    /// @note If the vector is parallel to the y-axis, the slope is infinite.
    element_t slope() const
    {
        if (abs(x()) <= precision_high<element_t>()) {
            return INFINITY;
        }
        return y() / x();
    }

    /// @brief Returns a vector orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
    /// @note The resulting vector is of the same magnitude as the original one.
    _RealVector2 orthogonal() const { return _RealVector2(-y(), x()); }

    /// @brief Returns the cross product of this vector and another.
    /// As defined at http://mathworld.wolfram.com/CrossProduct.html
    /// @param other     Vector to the right.
    element_t cross(const _RealVector2 other) const { return (x() * other.y()) - (y() * other.x()); }

    /// @brief Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    _RealVector2 project_on(const _RealVector2& other) { return other * dot(other); }

    /// @brief Returns a copy of this 2D Vector, rotated counter-clockwise by a given angle.
    /// @param angle    Angle in radians.
    _RealVector2 rotate(const element_t angle) const
    {
        const element_t sin_angle = sin(angle);
        const element_t cos_angle = cos(angle);
        return _RealVector2((x() * cos_angle) - (y() * sin_angle), (y() * cos_angle) + (x() * sin_angle));
    }

    /// @brief Returns a copy this vector rotated around a pivot point by a given angle.
    /// @param angle    Angle in radians.
    _RealVector2 rotate_around(const element_t angle, const _RealVector2& pivot) const
    {
        return (*this - pivot).rotate(angle) += pivot;
    }
};

//====================================================================================================================//

/** 2-dimensional mathematical vector containing integers. */
template<typename INTEGER>
struct _IntVector2 : public detail::Arithmetic<_IntVector2<INTEGER>, INTEGER, 2> {

    /// @brief Element type.
    using element_t = INTEGER;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_IntVector2<INTEGER>, INTEGER, 2>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _IntVector2() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    _IntVector2(const element_t x, const element_t y = 0) : super_t{x, y} {}

    /// @brief Unit vector along the X-axis.
    static _IntVector2 x_axis() { return _IntVector2(1, 0); }

    /// @brief Unit vector along the Y-axis.
    static _IntVector2 y_axis() { return _IntVector2(0, 1); }

    /// @brief Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// @brief Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// @brief Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// @brief Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// @brief Tests if this vector is parallel to the Y-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_vertical() const { return x() == 0; }

    /// @brief Tests if this vector is parallel to the X-axis.
    /// @note The zero vector is parallel to every vector.
    bool is_horizontal() const { return y() == 0; }

    /// @brief Returns a vector orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
    /// @note The resulting vector is of the same magnitude as the original one.
    _IntVector2 orthogonal() const { return _IntVector2(-y(), x()); }
};

//====================================================================================================================//

using Vector2f = _RealVector2<float>;
using Vector2d = _RealVector2<double>;
using Vector2h = _RealVector2<half>;
using Vector2i = _IntVector2<int>;

//====================================================================================================================//

/// @brief Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector2f& vec);
std::ostream& operator<<(std::ostream& out, const Vector2d& vec);
std::ostream& operator<<(std::ostream& out, const Vector2h& vec);
std::ostream& operator<<(std::ostream& out, const Vector2i& vec);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for notf::_RealVector2.
template<typename Real>
struct hash<notf::_RealVector2<Real>> {
    size_t operator()(const notf::_RealVector2<Real>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR2R), vector.hash());
    }
};

/// @brief std::hash specialization for notf::_IntVector2.
template<typename Integer>
struct hash<notf::_IntVector2<Integer>> {
    size_t operator()(const notf::_IntVector2<Integer>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR2I), vector.hash());
    }
};

} // namespace std
