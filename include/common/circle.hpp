#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/meta.hpp"
#include "common/vector2.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief 2D Circle shape.
template<typename Real, ENABLE_IF_REAL(Real)>
struct Circle {

    /// @brief Element type.
    using element_t = Real;

    /// @brief Component type.
    using component_t = RealVector2<Real>;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Center position of the Circle.
    RealVector2<Real> center;

    /// @brief Radius of the Circle.
    element_t radius;

    // methods -------------------------------------------------------------------------------------------------------//
    // @brief Default constructor.
    Circle() = default;

    /// @brief Constructs a Circle of the given radius, centered at the given coordinates.
    /// @param center    Center position of the Circle.
    /// @param radius    Radius of the Circle.
    Circle(component_t center, const element_t radius) : center(std::move(center)), radius(radius) {}

    /// @brief Constructs a Circle of the given radius, centered at the origin.
    /// @param radius    Radius of the Circle.
    Circle(const element_t radius) : center(component_t::zero()), radius(radius) {}

    /// @brief Produces a zero Circle.
    static Circle zero() { return {{0, 0}, 0}; }

    /// @brief Checks if this is a zero Circle.
    bool is_zero() const { return radius == 0.f; }

    /// @brief The diameter of the Circle.
    element_t diameter() const { return radius * 2.f; }

    /// @brief The circumenfence of this Circle.
    element_t circumfence() const { return pi<element_t>() * radius * 2.f; }

    /// @brief The area of this Circle.
    element_t area() const { return pi<element_t>() * radius * radius; }

    /// @brief Checks, if the given point is contained within (or on the border of) this Circle.
    /// @param point    Point to check.
    bool contains(const component_t& point) const { return (point - center).magnitude_sq() <= (radius * radius); }

    /// @brief Checks if the other Circle intersects with this one.
    /// Intersection requires the intersected area to be >= zero.
    /// @param other    Circle to intersect.
    bool intersects(const Circle& other) const
    {
        const element_t radii = radius + other.radius;
        return (other.center - center).magnitude_sq() < (radii * radii);
    }

    /// @brief Returns the closest point inside this Circle to the given target point.
    /// @param target   Target point.
    component_t closest_point_to(const component_t& target) const
    {
        const component_t delta = target - center;
        const element_t mag_sq  = delta.magnitude_sq();
        if (mag_sq <= (radius * radius)) {
            return target;
        }
        if (mag_sq == 0.f) {
            return center;
        }
        return (delta / sqrt(mag_sq)) * radius;
    }

    /// @brief Equality operator.
    bool operator==(const Circle& other) const { return (other.center == center && other.radius == radius); }

    /// @brief Inequality operator.
    bool operator!=(const Circle& other) const { return (other.center != center || other.radius != radius); }

    /// @brief Sets this Circle to zero.
    void set_zero()
    {
        center.set_zero();
        radius = 0.f;
    }
};

} // namespace detail

//====================================================================================================================//

using Circlef = detail::Circle<float>;

//====================================================================================================================//

/// @brief Prints the contents of a circle into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param circle   Circle to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Circlef& circle);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for Circle.
template<typename Real>
struct hash<notf::detail::Circle<Real>> {
    size_t operator()(const notf::detail::Circle<Real>& circle) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::CIRCLE), circle.center, circle.radius);
    }
};

} // namespace std
