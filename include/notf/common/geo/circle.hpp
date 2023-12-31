#pragma once

#include "notf/common/geo/vector2.hpp"

NOTF_OPEN_NAMESPACE

// circle =========================================================================================================== //

namespace detail {

/// 2D Circle shape.
template<class Element>
struct Circle {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type.
    using vector_t = detail::Vector2<Element>;

    /// Element type.
    using element_t = Element;

    // methods --------------------------------------------------------------------------------- //
public:
    // Default constructor.
    Circle() = default;

    /// Constructs a Circle of the given radius, centered at the given coordinates.
    /// @param center    Center position of the Circle.
    /// @param radius    Radius of the Circle.
    Circle(vector_t center, const element_t radius) : center(std::move(center)), radius(radius) {}

    /// Constructs a Circle of the given radius, centered at the origin.
    /// @param radius    Radius of the Circle.
    Circle(const element_t radius) : center(vector_t::zero()), radius(radius) {}

    /// Produces a zero Circle.
    static Circle zero() { return {{0, 0}, 0}; }

    /// Checks if this is a zero Circle.
    bool is_zero() const { return radius == 0.f; }

    /// The diameter of the Circle.
    element_t diameter() const { return radius * 2.f; }

    /// The circumenfence of this Circle.
    element_t circumfence() const { return pi<element_t>() * radius * 2.f; }

    /// The area of this Circle.
    element_t area() const { return pi<element_t>() * radius * radius; }

    /// Checks, if the given point is contained within (or on the border of) this Circle.
    /// @param point    Point to check.
    bool contains(const vector_t& point) const { return (point - center).magnitude_sq() <= (radius * radius); }

    /// Checks if the other Circle intersects with this one.
    /// Intersection requires the intersected area to be >= zero.
    /// @param other    Circle to intersect.
    bool intersects(const Circle& other) const {
        const element_t radii = radius + other.radius;
        return (other.center - center).magnitude_sq() < (radii * radii);
    }

    /// Returns the closest point inside this Circle to the given target point.
    /// @param target   Target point.
    vector_t closest_point_to(const vector_t& target) const {
        const vector_t delta = target - center;
        const element_t mag_sq = delta.magnitude_sq();
        if (mag_sq <= (radius * radius)) { return target; }
        if (mag_sq == 0.f) { return center; }
        return (delta / sqrt(mag_sq)) * radius;
    }

    /// Equality operator.
    bool operator==(const Circle& other) const { return (other.center == center && other.radius == radius); }

    /// Inequality operator.
    bool operator!=(const Circle& other) const { return (other.center != center || other.radius != radius); }

    /// Sets this Circle to zero.
    void set_zero() {
        center.set_zero();
        radius = 0.f;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Center position of the Circle.
    vector_t center;

    /// Radius of the Circle.
    element_t radius;
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a circle into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param circle   Circle to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Circlef& circle) { return out << fmt::format("{}", circle); }

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Circle<Element>> {
    using type = notf::detail::Circle<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& circle, FormatContext& ctx) {
        return format_to(ctx.begin(), "Circlef([{}, {}], {})", circle.center.x(), circle.center.y(), circle.radius);
        // TODO: circle printing is bad, missing get_type and weird formatting
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Circle.
template<typename REAL>
struct std::hash<notf::detail::Circle<REAL>> {
    size_t operator()(const notf::detail::Circle<REAL>& circle) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::CIRCLE), circle.center, circle.radius);
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::Circlef) == sizeof(notf::V2f) + sizeof(float));
static_assert(std::is_pod<notf::Circlef>::value);
