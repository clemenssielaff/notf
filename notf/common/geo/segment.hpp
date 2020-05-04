#pragma once

#include <iosfwd>

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/triangle.hpp"
#include "notf/common/geo/vector2.hpp"
#include "notf/common/geo/vector3.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// segment ========================================================================================================== //

/// Baseclass for oriented line Segments both 2D and 3D.
template<class Vector>
struct Segment {

    /// Vector type.
    using vector_t = Vector;

    /// Element type.
    using element_t = typename vector_t::element_t;

    // fields ---------------------------------------------------------------------------------- //
    /// Start point of the Segment.
    vector_t start;

    /// End point of the Segment.
    vector_t end;

    // methods --------------------------------------------------------------------------------- //
    /// Default constructor.
    Segment() = default;

    /// Value Constructor.
    /// @param start    Start point.
    /// @param end      End point.
    Segment(vector_t start, const vector_t& end) : start(std::move(start)), end(std::move(end)) {}

    /// Difference vector between the end and start point of the Segment.
    vector_t get_delta() const { return end - start; }

    /// Length of this line Segment.
    element_t get_length() const { return get_delta().magnitude(); }

    /// The squared length of this line Segment.
    element_t get_length_sq() const { return get_delta().magnitude_sq(); }

    /// Has the Segment zero length?
    bool is_zero() const { return get_delta().is_zero(); }

    /// The AABR of this line Segment.
    Aabrf get_bounding_rect() const { return {start, end}; }

    /// Checks whether this Segment is parallel to another.
    bool is_parallel_to(const Segment& other) const { return get_delta().is_parallel_to(other.delta()); }

    /// Checks whether this Segment is orthogonal to another.
    bool is_orthogonal_to(const Segment& other) const { return get_delta().is_orthogonal_to(other.delta()); }

    /// Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    bool contains(const vector_t& point) const {
        return abs((point - start).direction_to(point - end) + 1) <= precision_high<element_t>();
    }
};

// segment2 ========================================================================================================= //

/// 2D line segment.
template<class Element>
struct Segment2 : public Segment<detail::Vector2<Element>> {

    /// Base type.
    using super_t = Segment<detail::Vector2<Element>>;

    /// Vector type.
    using vector_t = typename super_t::vector_t;

    /// Element type.
    using element_t = typename super_t::element_t;

    // fields ---------------------------------------------------------------------------------- //
    /// Start point of the Segment.
    using super_t::start;

    /// End point of the Segment.
    using super_t::end;

    // methods ---------------------------------------------------------------------------------//

    using super_t::contains;
    using super_t::get_bounding_rect;
    using super_t::get_delta;
    using super_t::get_length;
    using super_t::get_length_sq;
    using super_t::is_orthogonal_to;
    using super_t::is_parallel_to;
    using super_t::is_zero;

    /// Default constructor.
    Segment2() = default;

    /// Value constructor.
    /// @param start    Start point.
    /// @param end      End point.
    Segment2(vector_t start, const vector_t& end) : super_t(std::move(start), std::move(end)) {}

    /// Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    bool contains(const vector_t& point) const {
        return Triangle<element_t>(start, end, point).is_zero() && ((point - start).dot(point - end) < 0);
    }

    /// Quick tests if this Segment intersects another one.
    /// Does not calculate the actual point of intersection, only whether the Segments intersect at all.
    /// @returns True iff the two Segments intersect.
    bool intersects(const Segment2& other) {
        return (Triangle<element_t>(start, end, other.start).orientation()
                != Triangle<element_t>(start, end, other.end).orientation())
               && (Triangle<element_t>(other.start, other.end, start).orientation()
                   != Triangle<element_t>(other.start, other.end, end).orientation());
    }

    /// Returns the point on this Line that is closest to a given position.
    /// If the line has a length of zero, the start point is returned.
    /// @param point     Position to find the closest point on this Line to.
    /// @param inside    If true, the closest point has to lie within this Line's segment (between start and end).
    vector_t get_closest_point(const vector_t& point, bool inside = true) const {
        const vector_t delta = get_delta();
        const float length_sq = delta.magnitude_sq();
        if (is_approx(length_sq, 0)) { return start; }
        const float dot = (point - start).dot(delta);
        float t = -dot / length_sq;
        if (inside) { t = clamp(t, 0.0, 1.0); }
        return start + (delta * t);
    }

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
    // TODO: Line2 intersect
};

// segment3 ========================================================================================================= //

/// 3D line segment.
template<class Element>
struct Segment3 : public Segment<detail::Vector3<Element>> {

    /// Base type.
    using super_t = Segment<detail::Vector3<Element>>;

    /// Vector type.
    using vector_t = typename super_t::vector_t;

    /// Element type.
    using element_t = typename super_t::element_t;

    // fields ---------------------------------------------------------------------------------- //
    /// Start point of the Segment.
    using super_t::start;

    /// End point of the Segment.
    using super_t::end;

    // methods --------------------------------------------------------------------------------- //
    /// Default constructor.
    Segment3() = default;
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a Segment into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param segment  Segment to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Segment2f& segment) {
    return out << fmt::format("{}", segment);
}
inline std::ostream& operator<<(std::ostream& out, const Segment3f& segment) {
    return out << fmt::format("{}", segment);
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Segment2<Element>> {
    using type = notf::detail::Segment2<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& segment, FormatContext& ctx) {
        return format_to(ctx.begin(), "Line2f([{}, {}] -> [{}, {}])", segment.start.x(), segment.start.y(),
                         segment.end.x(), segment.end.y());
        // TODO: use get_type with Segment2 formatting
    }
};

template<class Element>
struct formatter<notf::detail::Segment3<Element>> {
    using type = notf::detail::Segment3<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& segment, FormatContext& ctx) {
        return format_to(ctx.begin(), "Line3f([{}, {}] -> [{}, {}])", segment.start.x(), segment.start.y(),
                         segment.end.x(), segment.end.y());
        // TODO: use get_type with Segment3 formatting
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Segment.
template<typename VECTOR>
struct std::hash<notf::detail::Segment<VECTOR>> {
    size_t operator()(const notf::detail::Segment<VECTOR>& segment) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::SEGMENT), segment.start, segment.end);
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::Segment2f) == sizeof(float) * 4);
static_assert(std::is_pod<notf::Segment2f>::value);
static_assert(sizeof(notf::Segment3f) == sizeof(float) * 6);
static_assert(std::is_pod<notf::Segment3f>::value);
