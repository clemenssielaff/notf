#pragma once

#include <iosfwd>

#include "common/aabr.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief Line baseclass.
template<typename VECTOR>
struct Line {

    /// @brief Vector type.
    using vector_t = VECTOR;

    /// @brief Element type.
    using element_t = typename vector_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Line.
    vector_t _start;

    /// @brief Vector from the start of the Line to its end point.
    vector_t _delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Line() = default;

    /// @brief Creates a Line from a start- and an end-point.
    /// @param start    Start point.
    /// @param end      End point.
    static Line from_points(vector_t start, const vector_t& end) { return {std::move(start), end - start}; }

    /// @brief Start point of the Line.
    const vector_t& start() const { return _start; }

    /// @brief Difference vector between the end and start point of the Line.
    const vector_t& delta() const { return _delta; }

    /// @brief End point of the Line.
    vector_t end() const { return _start + _delta; }

    /// @brief Length of this Line.
    element_t length() const { return _delta.magnitude(); }

    /// @brief The squared length of this Line.
    element_t length_sq() const { return _delta.magnitude_sq(); }

    /// @brief Has the Line zero length?
    bool is_zero() const { return _delta.is_zero(); }

    /// @brief The AABR of this Line.
    Aabrf bounding_rect() const { return {_start, std::move(end())}; }

    /// @brief Sets a new start point for this Line.
    /// Updates the complete Line - if you have a choice, favor setting the end point rather than the start point.
    /// @param start    New start point.
    Line& set_start(vector_t start)
    {
        _delta = end() - start;
        _start = std::move(start);
        return *this;
    }

    /// @brief Sets a new end point for this Line.
    /// @param end  New end point.
    Line& set_end(const vector_t& end)
    {
        _delta = end - _start;
        return *this;
    }

    /** Checks whether this Line is parallel to another. */
    bool is_parallel_to(const Line& other) const { return _delta.is_parallel_to(other._delta); }

    /** Checks whether this Line is orthogonal to another. */
    bool is_orthogonal_to(const Line& other) const { return _delta.is_orthogonal_to(other._delta); }
};

//====================================================================================================================//

/// @brief 2D line segment.
template<typename REAL>
struct Line2 : public detail::Line<detail::RealVector2<REAL>> {
    /// @brief Base type.
    using super_t = detail::Line<detail::RealVector2<REAL>>;

    /// @brief Vector type.
    using vector_t = typename super_t::vector_t;

    /// @brief Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Line.
    using super_t::_start;

    /// @brief Vector from the start of the Line to its end point.
    using super_t::_delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Line2() = default;

    /// @brief The x-coordinate where this Line crosses a given y-coordinate as it extendeds into infinity.
    /// If the Line is parallel to the x-axis, the returned coordinate is NAN.
    element_t x_at(element_t y) const
    {
        if (_delta.y() == approx(0)) {
            return NAN;
        }
        const element_t factor = (y - _start.y()) / _delta.y();
        return _start.x() + (_delta.x() * factor);
    }

    /// @brief The y-coordinate where this Line crosses a given x-coordinate as it extendeds into infinity.
    /// If the Line is parallel to the y-axis, the returned coordinate is NAN.
    element_t y_at(element_t x) const
    {
        if (_delta.x() == approx(0)) {
            return NAN;
        }
        const element_t factor = (x - _start.x()) / _delta.x();
        return _start.y() + (_delta.y() * factor);
    }

    /// @brief Returns the point on this Line that is closest to a given position.
    /// If the line has a length of zero, the start point is returned.
    /// @param point     Position to find the closest point on this Line to.
    /// @param inside    If true, the closest point has to lie within this Line's segment (between start and end).
//    vector_t closest_point(const vector_t& point, bool inside = true) const;

    /// @brief Calculates the intersection of this line with another ... IF they intersect.
    /// @param intersection  Out argument, is filled wil the intersection point if an intersection was found.
    /// @param other         Line to find an intersection with.
    /// @param in_self       True, if the intersection point has to lie within the bounds of this Line.
    /// @param in_other      True, if the intersection point has to lie within the bounds of the other Line.
    /// @return  False if the lines do not intersect.
    ///          If true, the intersection point ist written into the `intersection` argument.
//    bool
//    intersect(vector_t& intersection, const Line2& other, const bool in_self = true, const bool in_other = true) const;
};

//====================================================================================================================//

/// @brief 3D line segment.
template<typename REAL>
struct Line3 : public detail::Line<detail::RealVector3<REAL>> {
    /// @brief Base type.
    using super_t = detail::Line<detail::RealVector3<REAL>>;

    /// @brief Vector type.
    using vector_t = typename super_t::vector_t;

    /// @brief Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Line.
    using super_t::_start;

    /// @brief Vector from the start of the Line to its end point.
    using super_t::_delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Line3() = default;
};

} // namespace detail

//====================================================================================================================//

using Line2f = detail::Line2<float>;
using Line3f = detail::Line3<float>;

//====================================================================================================================//

/// @brief Prints the contents of a Line into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param line Line to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Line2f& line);
std::ostream& operator<<(std::ostream& out, const Line3f& line);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for notf::Line2.
template<typename REAL>
struct hash<notf::detail::Line2<REAL>> {
    size_t operator()(const notf::detail::Line2<REAL>& line) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::LINE), line.start(), line.delta());
    }
};

/// @brief std::hash specialization for notf::Line3.
template<typename REAL>
struct hash<notf::detail::Line3<REAL>> {
    size_t operator()(const notf::detail::Line3<REAL>& line) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::LINE), line.start(), line.delta());
    }
};

} // namespace std
