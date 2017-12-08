#pragma once

#include <iosfwd>

#include "common/aabr.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief Baseclass for oriented line Segments.
template<typename VECTOR>
struct Segment {

    /// @brief Vector type.
    using vector_t = VECTOR;

    /// @brief Element type.
    using element_t = typename vector_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Segment.
    vector_t _start;

    /// @brief Vector from the start of the Segment to its end point.
    vector_t _delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Segment() = default;

    /// @brief Creates a line Segment from a start- and an end-point.
    /// @param start    Start point.
    /// @param end      End point.
    static Segment from_points(vector_t start, const vector_t& end) { return {std::move(start), end - start}; }

    /// @brief Start point of the Segment.
    const vector_t& start() const { return _start; }

    /// @brief Difference vector between the end and start point of the Segment.
    const vector_t& delta() const { return _delta; }

    /// @brief End point of the Segment.
    vector_t end() const { return _start + _delta; }

    /// @brief Length of this line Segment.
    element_t length() const { return _delta.magnitude(); }

    /// @brief The squared length of this line Segment.
    element_t length_sq() const { return _delta.magnitude_sq(); }

    /// @brief Has the Segment zero length?
    bool is_zero() const { return _delta.is_zero(); }

    /// @brief The AABR of this line Segment.
    Aabrf bounding_rect() const { return {_start, std::move(end())}; }

    /// @brief Sets a new start point for this Segment.
    /// Updates the complete Segment - if you have a choice, favor setting the end point rather than the start point.
    /// @param start    New start point.
    Segment& set_start(vector_t start)
    {
        _delta = end() - start;
        _start = std::move(start);
        return *this;
    }

    /// @brief Sets a new end point for this Segment.
    /// @param end  New end point.
    Segment& set_end(const vector_t& end)
    {
        _delta = end - _start;
        return *this;
    }

    /// @brief Checks whether this Segment is parallel to another.
    bool is_parallel_to(const Segment& other) const { return _delta.is_parallel_to(other._delta); }

    /// @brief Checks whether this Segment is orthogonal to another.
    bool is_orthogonal_to(const Segment& other) const { return _delta.is_orthogonal_to(other._delta); }

    /// @brief Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    bool contains(const vector_t& point) const
    {
        const vector_t point_delta = point - _start;
        return abs(_delta.direction_to(point_delta) - 1) <= precision_high<element_t>()
               && (point_delta.magnitude_sq <= _delta.magnitude_sq());
    }
};

//====================================================================================================================//

/// @brief 2D line segment.
template<typename REAL>
struct Segment2 : public detail::Segment<detail::RealVector2<REAL>> {
    /// @brief Base type.
    using super_t = detail::Segment<detail::RealVector2<REAL>>;

    /// @brief Vector type.
    using vector_t = typename super_t::vector_t;

    /// @brief Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Segment.
    using super_t::_start;

    /// @brief Vector from the start of the Segment to its end point.
    using super_t::_delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Segment2() = default;

    /// @brief The x-coordinate where this Segment crosses a given y-coordinate as it extendeds into infinity.
    /// If the Segment is parallel to the x-axis, the returned coordinate is NAN.
    element_t x_at(element_t y) const
    {
        if (_delta.y() == approx(0)) {
            return NAN;
        }
        const element_t factor = (y - _start.y()) / _delta.y();
        return _start.x() + (_delta.x() * factor);
    }

    /// @brief The y-coordinate where this Segment crosses a given x-coordinate as it extendeds into infinity.
    /// If the Segment is parallel to the y-axis, the returned coordinate is NAN.
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
    //    intersect(vector_t& intersection, const Line2& other, const bool in_self = true, const bool in_other = true)
    //    const;
};

//====================================================================================================================//

/// @brief 3D line segment.
template<typename REAL>
struct Segment3 : public detail::Segment<detail::RealVector3<REAL>> {
    /// @brief Base type.
    using super_t = detail::Segment<detail::RealVector3<REAL>>;

    /// @brief Vector type.
    using vector_t = typename super_t::vector_t;

    /// @brief Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Start point of the Segment.
    using super_t::_start;

    /// @brief Vector from the start of the Segment to its end point.
    using super_t::_delta;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Segment3() = default;
};

} // namespace detail

//====================================================================================================================//

using Segment2f = detail::Segment2<float>;
using Segment3f = detail::Segment3<float>;

//====================================================================================================================//

/// @brief Prints the contents of a Segment into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param segment  Segment to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Segment2f& segment);
std::ostream& operator<<(std::ostream& out, const Segment3f& segment);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for Segment.
template<typename VECTOR>
struct hash<notf::detail::Segment<VECTOR>> {
    size_t operator()(const notf::detail::Segment<VECTOR>& segment) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::SEGMENT), segment.start(), segment.delta());
    }
};

} // namespace std
