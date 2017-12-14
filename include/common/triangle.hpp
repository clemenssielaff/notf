#pragma once

#include <iosfwd>

#include "common/vector2.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief Baseclass for Triangles.
template<typename REAL>
struct Triangle {

    /// @brief Vector type.
    using vector_t = RealVector2<REAL>;

    /// @brief Element type.
    using element_t = typename vector_t::element_t;

    // types ---------------------------------------------------------------------------------------------------------//

    /// @brief Orientation of the Triangle.
    enum class Orientation : char {
        COUNTERCLOCKWISE,
        CLOCKWISE,
        CCW = COUNTERCLOCKWISE,
        CW  = CLOCKWISE,
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief First point of the Triangle.
    vector_t a;

    /// @brief Second point of the Triangle.
    vector_t b;

    /// @brief Third point of the Triangle.
    vector_t c;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Triangle() = default;

    /// @brief Value constructor.
    /// @param a    First point.
    /// @param b    Second point.
    /// @param c    Third point.
    Triangle(vector_t a, vector_t b, vector_t c) : a(std::move(a)), b(std::move(b)), c(std::move(c)) {}

    /// @brief The center point of the Triangle.
    vector_t center() const { return (a + b + c) / 3; }

    /// @brief Checks whether the Triangle has a zero area.
    bool is_zero() const { return abs(_signed_half_area()) < precision_high<element_t>(); }

    /// @brief Area of this Triangle.
    element_t area() const { return abs(_signed_half_area()) / 2; }

    /// @brief Orientation of this Triangle (zero Triangle is CCW).
    Orientation orientation() const { return _signed_half_area() >= 0 ? Orientation::CCW : Orientation::CW; }

    /// @brief Tests whether this Triangle contains a given point.
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

/// @brief Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param triangle Triangle to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Trianglef& triangle);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for Triangle.
template<typename REAL>
struct hash<notf::detail::Triangle<REAL>> {
    size_t operator()(const notf::detail::Triangle<REAL>& triangle) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::TRIANGLE), triangle.a, triangle.b, triangle.c);
    }
};

} // namespace std
