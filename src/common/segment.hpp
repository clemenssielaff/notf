#pragma once

#include <iosfwd>

#include "common/aabr.hpp"
#include "common/triangle.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

//====================================================================================================================//

/// Baseclass for oriented line Segments both 2D and 3D.
template<typename VECTOR>
struct Segment {

    /// Vector type.
    using vector_t = VECTOR;

    /// Element type.
    using element_t = typename vector_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Start point of the Segment.
    vector_t start;

    /// End point of the Segment.
    vector_t end;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    Segment() = default;

    /// Value Constructor.
    /// @param start    Start point.
    /// @param end      End point.
    Segment(vector_t start, const vector_t& end) : start(std::move(start)), end(std::move(end)) {}

    /// Difference vector between the end and start point of the Segment.
    const vector_t& delta() const { return end - start; }

    /// Length of this line Segment.
    element_t length() const { return delta().magnitude(); }

    /// The squared length of this line Segment.
    element_t length_sq() const { return delta().magnitude_sq(); }

    /// Has the Segment zero length?
    bool is_zero() const { return delta().is_zero(); }

    /// The AABR of this line Segment.
    Aabrf bounding_rect() const { return {start, end}; }

    /// Checks whether this Segment is parallel to another.
    bool is_parallel_to(const Segment& other) const { return delta().is_parallel_to(other.delta()); }

    /// Checks whether this Segment is orthogonal to another.
    bool is_orthogonal_to(const Segment& other) const { return delta().is_orthogonal_to(other.delta()); }

    /// Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    bool contains(const vector_t& point) const
    {
        return abs((point - start).direction_to(point - end) + 1) <= precision_high<element_t>();
    }
};

//====================================================================================================================//

/// 2D line segment.
template<typename REAL>
struct Segment2 : public detail::Segment<detail::RealVector2<REAL>> {

    /// Base type.
    using super_t = detail::Segment<detail::RealVector2<REAL>>;

    /// Vector type.
    using vector_t = typename super_t::vector_t;

    /// Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Start point of the Segment.
    using super_t::start;

    /// End point of the Segment.
    using super_t::end;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    Segment2() = default;

    /// Value constructor.
    /// @param start    Start point.
    /// @param end      End point.
    Segment2(vector_t start, const vector_t& end) : super_t(std::move(start), std::move(end)) {}

    /// Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    bool contains(const vector_t& point) const
    {
        return Triangle<element_t>(start, end, point).is_zero() && ((point - start).dot(point - end) < 0);
    }

    /// Quick tests if this Segment intersects another one.
    /// Does not calculate the actual point of intersection, only whether the Segments intersect at all.
    /// @returns True iff the two Segments intersect.
    bool intersects(const Segment2& other)
    {
        return (Triangle<element_t>(start, end, other.start).orientation()
                != Triangle<element_t>(start, end, other.end).orientation())
               && (Triangle<element_t>(other.start, other.end, start).orientation()
                   != Triangle<element_t>(other.start, other.end, end).orientation());
    }

    /// Returns the point on this Line that is closest to a given position.
    /// If the line has a length of zero, the start point is returned.
    /// @param point     Position to find the closest point on this Line to.
    /// @param inside    If true, the closest point has to lie within this Line's segment (between start and end).
    //    vector_t closest_point(const vector_t& point, bool inside = true) const;

    /// Calculates the intersection of this line with another ... IF they intersect.
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

/// 3D line segment.
template<typename REAL>
struct Segment3 : public detail::Segment<detail::RealVector3<REAL>> {

    /// Base type.
    using super_t = detail::Segment<detail::RealVector3<REAL>>;

    /// Vector type.
    using vector_t = typename super_t::vector_t;

    /// Element type.
    using element_t = typename super_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Start point of the Segment.
    using super_t::start;

    /// End point of the Segment.
    using super_t::end;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    Segment3() = default;
};

} // namespace detail

//====================================================================================================================//

using Segment2f = detail::Segment2<float>;
using Segment3f = detail::Segment3<float>;

//====================================================================================================================//

/// Prints the contents of a Segment into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param segment  Segment to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Segment2f& segment);
std::ostream& operator<<(std::ostream& out, const Segment3f& segment);

NOTF_CLOSE_NAMESPACE

//====================================================================================================================//

namespace std {

/// std::hash specialization for Segment.
template<typename VECTOR>
struct hash<notf::detail::Segment<VECTOR>> {
    size_t operator()(const notf::detail::Segment<VECTOR>& segment) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::SEGMENT), segment.start, segment.end);
    }
};

} // namespace std
