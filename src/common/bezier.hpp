#pragma once

#include "common/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// ================================================================================================================== //

/// Bezier spline.
template<size_t ORDER, typename VECTOR>
struct Bezier {

    /// Vector type.
    using vector_t = VECTOR;

    /// Element type.
    using element_t = typename VECTOR::element_t;

    /// Order of this Bezier spline.
    static constexpr size_t order() { return ORDER; }

    // types -------------------------------------------------------------------------------------------------------- //

    // TODO: Segments should share points, no need to store the start of one and the end of another

    struct Segment {
        // fields --------------------------------------------------------------------------------------------------- //
        /// Start of the spline, in absolute coordinates.
        vector_t start;

        /// First control point, in absolute coordinates.
        vector_t ctrl1;

        /// Second control point, in absolute coordinates.
        vector_t ctrl2;

        /// End of the spline, in absolute coordinates.
        vector_t end;

        // TODO: proper bezier template class (dimensions & order)
        // std::array<component_t, order() + 1> points;

        // methods -------------------------------------------------------------------------------------------------- //
        /// Default constructor.
        Segment() = default;

        /// Element-wise constructor.
        /// @param start    Start of the spline, in absolute coordinates.
        /// @param ctrl1    First control point, in absolute coordinates.
        /// @param ctrl2    Second control point, in absolute coordinates.
        /// @param end      End of the spline, in absolute coordinates.
        Segment(const vector_t a, const vector_t b, const vector_t c, const vector_t d)
            : start(std::move(a)), ctrl1(std::move(b)), ctrl2(std::move(c)), end(std::move(d))
        {}

        /// Straight line.
        /// @param start    Start of the spline, in absolute coordinates.
        /// @param end      End of the spline, in absolute coordinates.
        static Segment line(const vector_t a, const vector_t d)
        {
            const vector_t delta_thirds = (d - a) * (1. / 3.);
            return Segment(a, a + delta_thirds, a + (delta_thirds * 2), std::move(d));
        }

        vector_t tangent(element_t t) const
        {
            // the tangent at the very extremes 0 and 1 may not be defined
            static const element_t epsilon = std::numeric_limits<element_t>::epsilon() * 100;
            t                              = clamp(t, epsilon, 1 - epsilon);

            const element_t ti = 1 - t;
            return ((ctrl1 - start) * (3 * ti * ti)) + ((ctrl2 - ctrl1) * (6 * ti * t)) + ((end - ctrl2) * (3 * t * t));
        }
    };

    // methods ------------------------------------------------------------------------------------------------------ //

    Bezier() = default;

    Bezier(std::vector<Segment> segments) : segments(std::move(segments)) {}

    // fields ------------------------------------------------------------------------------------------------------- //

    std::vector<Segment> segments;
};

} // namespace detail

// ================================================================================================================== //

using CubicBezier2f = detail::Bezier<3, Vector2f>;

// ================================================================================================================== //

/// Prints the contents of a Bezier spline into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param bezier   Bezier spline to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const CubicBezier2f& bezier);

NOTF_CLOSE_NAMESPACE

// ================================================================================================================== //

namespace std {

/// std::hash specialization for Bezier.
template<size_t ORDER, typename VECTOR>
struct hash<notf::detail::Bezier<ORDER, VECTOR>> {
    size_t operator()(const notf::detail::Bezier<ORDER, VECTOR>& bezier) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::BEZIER), bezier.order(), bezier.start.hash(),
                          bezier.ctrl1.hash(), bezier.ctrl2.hash(), bezier.end.hash());
    }
};

} // namespace std
