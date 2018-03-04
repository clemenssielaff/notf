#pragma once

#include <iosfwd>

#include "common/vector2.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// Baseclass for Triangles.
template<typename REAL>
struct Triangle {

    /// Vector type.
    using vector_t = RealVector2<REAL>;

    /// Element type.
    using element_t = typename vector_t::element_t;

    // types ---------------------------------------------------------------------------------------------------------//

    /// Orientation of the Triangle.
    enum class Orientation : char {
        CCW              = 1,
        CW               = 2,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE        = CW,
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// First point of the Triangle.
    vector_t a;

    /// Second point of the Triangle.
    vector_t b;

    /// Third point of the Triangle.
    vector_t c;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    Triangle() = default;

    /// Value constructor.
    /// @param a    First point.
    /// @param b    Second point.
    /// @param c    Third point.
    Triangle(vector_t a, vector_t b, vector_t c) : a(std::move(a)), b(std::move(b)), c(std::move(c)) {}

    /// The center point of the Triangle.
    vector_t center() const { return (a + b + c) / 3; }

    /// Checks whether the Triangle has a zero area.
    bool is_zero() const { return abs(_signed_half_area(a, b, c)) < precision_high<element_t>(); }

    /// Area of this Triangle.
    element_t area() const { return abs(_signed_half_area(a, b, c)) / 2; }

    /// Orientation of this Triangle (zero Triangle is CCW).
    Orientation orientation() const { return _signed_half_area(a, b, c) >= 0 ? Orientation::CCW : Orientation::CW; }

    /// Tests whether this Triangle contains a given point.
    /// @param point    Point to test.
    bool contains(const vector_t& point) const
    {
        return (sign(_signed_half_area(a, b, point)) == sign(_signed_half_area(b, c, point)))
               && (sign(_signed_half_area(b, c, point)) == sign(_signed_half_area(c, a, point)));
    }

private:
    static element_t _signed_half_area(const vector_t& a, const vector_t& b, const vector_t& c)
    {
        return a.x() * (b.y() - c.y()) + b.x() * (c.y() - a.y()) + c.x() * (a.y() - b.y());
    }
};

} // namespace detail

//====================================================================================================================//

using Trianglef = detail::Triangle<float>;

//====================================================================================================================//

/// Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param triangle Triangle to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Trianglef& triangle);

} // namespace notf

//====================================================================================================================//

namespace std {

/// std::hash specialization for Triangle.
template<typename REAL>
struct hash<notf::detail::Triangle<REAL>> {
    size_t operator()(const notf::detail::Triangle<REAL>& triangle) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::TRIANGLE), triangle.a, triangle.b, triangle.c);
    }
};

} // namespace std
