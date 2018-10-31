#pragma once

#include "notf/common/arithmetic.hpp"

NOTF_OPEN_NAMESPACE

// vector2 ========================================================================================================== //

namespace detail {

/// 2-dimensional mathematical Vector containing real numbers.
template<class Element>
struct Vector2 : public detail::ArithmeticImpl<Vector2<Element>, Element, Element, 2> {

    // types --------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = detail::ArithmeticImpl<Vector2<Element>, Element, Element, 2>;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    Vector2() = default;

    /// Forwarding constructor.
    template<class... Args>
    Vector2(Args&&... args) : super_t(std::forward<Args>(args)...)
    {}

    /// Unit vector along the X-axis.
    static Vector2 x_axis() { return Vector2(1, 0); }

    /// Unit vector along the Y-axis.
    static Vector2 y_axis() { return Vector2(0, 1); }

    /// Access to the first element in the vector.
    constexpr element_t& x() noexcept { return data[0]; }
    constexpr const element_t& x() const noexcept { return data[0]; }

    /// Access to the second element in the vector.
    constexpr element_t& y() noexcept { return data[1]; }
    constexpr const element_t& y() const noexcept { return data[1]; }

    /// Swizzles.
    constexpr Vector2 xy() const { return {data[0], data[1]}; }
    constexpr Vector2 yx() const { return {data[1], data[0]}; }

    /// Returns True, if other and self are approximately the same vector.
    /// Vectors use distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed distance between the two Vectors.
    constexpr bool is_approx(const Vector2& other, const element_t epsilon = precision_high<element_t>()) const noexcept
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /// Tests if this vector is parallel to the Y-axis.
    /// @note The zero vector is parallel to every vector.
    constexpr bool is_vertical() const noexcept { return abs(x()) <= precision_high<element_t>(); }

    /// Tests if this vector is parallel to the X-axis.
    /// @note The zero vector is parallel to every vector.
    constexpr bool is_horizontal() const noexcept { return abs(y()) <= precision_high<element_t>(); }

    /// Checks whether this vector's direction is parallel to the other.
    /// @note The zero vector is parallel to every other vector.
    /// @param other    Vector to test against.
    constexpr bool is_parallel_to(const Vector2& other) const noexcept
    {
        return cross(other) <= precision_high<element_t>();
    }

    /// Returns the smallest angle (in radians) to the other vector.
    /// @note Always returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t get_angle_to(const Vector2& other) const
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
    element_t get_direction_to(const Vector2& other) const
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

    /// Returns a vector orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
    /// @note The resulting vector is of the same magnitude as the original one.
    constexpr Vector2 get_orthogonal() const noexcept { return Vector2(-y(), x()); }

    /// Returns the cross product of this vector and another.
    /// As defined at http://mathworld.wolfram.com/CrossProduct.html
    /// Treats the 2D vectors like 3D vectors with z-components equal to zeros, takes their cross product, and returns
    /// the z-component of the result.
    /// @param other     Vector to the right.
    constexpr element_t cross(const Vector2 other) const noexcept { return (x() * other.y()) - (y() * other.x()); }

    /// Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    constexpr Vector2 project_on(const Vector2& other) const noexcept { return other * dot(other); }

    /// Returns a copy of this 2D Vector, rotated counter-clockwise by a given angle.
    /// @param angle    Angle in radians.
    constexpr Vector2 get_rotated(const element_t angle) const noexcept
    {
        const element_t sin_angle = sin(angle);
        const element_t cos_angle = cos(angle);
        return Vector2((x() * cos_angle) - (y() * sin_angle), (y() * cos_angle) + (x() * sin_angle));
    }

    /// Returns a copy this vector rotated around a pivot point by a given angle.
    /// @param angle    Angle in radians.
    /// @param pivot    Position around which to rotate
    constexpr Vector2 get_rotated(const element_t angle, const Vector2& pivot) const noexcept
    {
        return (*this - pivot).get_rotated(angle) += pivot;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

using V2f = detail::Vector2<float>;
using V2d = detail::Vector2<double>;
using V2i = detail::Vector2<int>;
using V2s = detail::Vector2<short>;

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const V2f& vec);
std::ostream& operator<<(std::ostream& out, const V2d& vec);
std::ostream& operator<<(std::ostream& out, const V2i& vec);
std::ostream& operator<<(std::ostream& out, const V2s& vec);

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for Vector2.
template<class Element>
struct hash<notf::detail::Vector2<Element>> {
    size_t operator()(const notf::detail::Vector2<Element>& vector) const
    {
        return notf::hash(notf::to_number(notf::detail::HashID::VECTOR2), vector.hash());
    }
};

} // namespace std
