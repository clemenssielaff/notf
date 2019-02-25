#pragma once

#include "notf/common/geo/vector2.hpp"

NOTF_OPEN_NAMESPACE

// orientation ====================================================================================================== //

/// Orientation of the Triangle.
enum class Orientation : char {
    CCW = 1,
    CW = 2,
    COUNTERCLOCKWISE = CCW,
    CLOCKWISE = CW,
    SOLID = CCW,
    HOLE = CW,
};

/// Inverse Orientation.
inline constexpr Orientation operator-(const Orientation orientation) noexcept {
    return (orientation == Orientation::CCW) ? Orientation::CW : Orientation::CCW;
}

// triangle ========================================================================================================= //

namespace detail {

/// Baseclass for Triangles.
template<class Element>
struct Triangle {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type.
    using vector_t = detail::Vector2<Element>;

    /// Element type.
    using element_t = typename vector_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Triangle() noexcept = default;

    /// Value constructor.
    /// @param a    First point.
    /// @param b    Second point.
    /// @param c    Third point.
    constexpr Triangle(vector_t a, vector_t b, vector_t c) noexcept
        : a(std::move(a)), b(std::move(b)), c(std::move(c)) {}

    /// The center point of the Triangle.
    constexpr vector_t get_center() const noexcept { return (a + b + c) /= 3; }

    /// Checks whether the Triangle has a zero area.
    constexpr bool is_degenerate() const noexcept {
        return abs(_twice_signed_area(a, b, c)) < precision_high<element_t>();
    }

    /// Area of this Triangle, is always positive.
    constexpr element_t get_area() const noexcept { return abs(_twice_signed_area(a, b, c)) / 2; }

    /// Signed area of this Triangle.
    /// The area is positive if the orientation of the triangle is counterclockwise and negative if it is clockwise.
    constexpr element_t get_signed_area() const noexcept { return _twice_signed_area(a, b, c) / 2; }

    /// Orientation of this Triangle (zero Triangle is CCW).
    constexpr Orientation get_orientation() const noexcept {
        return _twice_signed_area(a, b, c) >= 0 ? Orientation::CCW : Orientation::CW;
    }

    /// Tests whether this Triangle contains a given point.
    /// @param point    Point to test.
    constexpr bool contains(const vector_t& point) const noexcept {
        return (sign(_twice_signed_area(a, b, point)) == sign(_twice_signed_area(b, c, point)))
               && (sign(_twice_signed_area(b, c, point)) == sign(_twice_signed_area(c, a, point)));
    }

private:
    /// Equivalent to the Shoelace formula:
    /// https://en.wikipedia.org/wiki/Shoelace_formula#Proof_for_a_triangle
    /// but optimized for computation (see https://godbolt.org/z/ZuUDZV).
    constexpr static element_t _twice_signed_area(const vector_t& a, const vector_t& b, const vector_t& c) noexcept {
        return a.x() * (b.y() - c.y()) + b.x() * (c.y() - a.y()) + c.x() * (a.y() - b.y());
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// First point of the Triangle.
    vector_t a;

    /// Second point of the Triangle.
    vector_t b;

    /// Third point of the Triangle.
    vector_t c;
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param triangle Triangle to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Trianglef& triangle);

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Triangle<Element>> {
    using type = notf::detail::Triangle<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& triangle, FormatContext& ctx) {
        return format_to(ctx.begin(), "Trianglef([{}, {}], [{}, {}], [{}, {}])", triangle.a.x(), triangle.a.y(),
                         triangle.b.x(), triangle.b.y(), triangle.c.x(), triangle.c.y());
        // TODO use get_type for Triangle formatting
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Triangle.
template<typename REAL>
struct std::hash<notf::detail::Triangle<REAL>> {
    size_t operator()(const notf::detail::Triangle<REAL>& triangle) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::TRIANGLE), triangle.a, triangle.b, triangle.c);
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::Trianglef) == sizeof(notf::V2f) * 3);
static_assert(std::is_pod<notf::Trianglef>::value);
