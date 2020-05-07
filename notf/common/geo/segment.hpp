#pragma once

#include <iosfwd>

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/triangle.hpp"
#include "notf/common/geo/vector2.hpp"

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
    Segment(vector_t start, vector_t end) : start(std::move(start)), end(std::move(end)) {}

    /// Difference vector between the end and start point of the Segment.
    vector_t get_delta() const noexcept { return end - start; }

    /// The length of this line Segment.
    element_t get_length() const noexcept { return get_delta().get_magnitude(); }

    /// The squared length of this line Segment.
    element_t get_length_sq() const noexcept { return get_delta().get_magnitude_sq(); }

    /// Checks whether the Segment zero length.
    bool is_zero(const element_t epsilon = precision_high<element_t>()) const noexcept {
        return get_delta().is_zero(epsilon);
    }

    /// Checks whether this Segment is parallel to another.
    bool is_parallel_to(const Segment& other) const noexcept { return get_delta().is_parallel_to(other.get_delta()); }

    /// Checks whether this Segment is orthogonal to another.
    bool is_orthogonal_to(const Segment& other) const noexcept {
        return get_delta().is_orthogonal_to(other.get_delta());
    }

    /// Checks if this line Segment contains a given point.
    /// @param point    Point to heck.
    constexpr bool contains(const vector_t& point) const noexcept {
        return abs((point - start).get_direction_to(point - end) + 1) <= precision_high<element_t>();
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
    /// Start point of the line Segment.
    using super_t::start;

    /// End point of the line Segment.
    using super_t::end;

    // methods ---------------------------------------------------------------------------------//

    using super_t::contains;
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
    Segment2(vector_t start, const vector_t& end) noexcept : super_t(std::move(start), std::move(end)) {}

    /// The Aabr of this line Segment.
    Aabr<element_t> get_bounding_rect() const noexcept { return {start, end}; }

    /// Checks if this line Segment contains a given point.
    /// Overrides the more general method in the Segment base class.
    /// @param point    Point to check.
    constexpr bool contains(const vector_t& point) const noexcept {
        return Triangle<element_t>(start, end, point).is_zero() && ((point - start).dot(point - end) < 0);
    }

    /// Quick tests if this line Segment intersects another one.
    /// Does not calculate the actual point of intersection, only whether the Segments intersect at all.
    /// @returns True iff the two Segments intersect.
    constexpr bool intersects(const Segment2& other) const noexcept {
        return (Triangle<element_t>(start, end, other.start).get_orientation()
                != Triangle<element_t>(start, end, other.end).get_orientation())
               && (Triangle<element_t>(other.start, other.end, start).get_orientation()
                   != Triangle<element_t>(other.start, other.end, end).get_orientation());
    }

    /// The position on this line Segment that is closest to a given point.
    /// If the line has a length of zero, the start point is returned.
    /// @param point     Position to find the closest point on this Line to.
    /// @param inside    If true, the closest point has to lie within this Line's segment (between start and end).
    vector_t get_closest_point(const vector_t& point, bool inside = true) const noexcept {
        const vector_t delta = get_delta();
        const float length_sq = delta.get_magnitude_sq();
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

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a Segment into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param segment  Segment to print.
/// @return Output stream for further output.
template<class Element>
inline std::ostream& operator<<(std::ostream& out, const notf::detail::Segment2<Element>& segment) {
    return out << fmt::format("{}", segment);
}

NOTF_CLOSE_NAMESPACE

template<class Element>
struct fmt::formatter<notf::detail::Segment2<Element>> {
    using type = notf::detail::Segment2<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& segment, FormatContext& ctx) {
        return format_to(ctx.begin(), "Segment2([{}, {}] -> [{}, {}])", segment.start.x(), segment.start.y(),
                         segment.end.x(), segment.end.y());
    }
};

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