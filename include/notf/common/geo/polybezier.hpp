#pragma once

#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/polyline.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// polybezier ======================================================================================================= //

template<size_t Order, class Vector>
class PolyBezier {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type used to define the position of a point.
    using vector_t = Vector;

    /// Element type.
    using element_t = typename vector_t::element_t;

    /// Polyline used as the hull of the PolyBezier.
    using hull_t = Polyline<element_t>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr PolyBezier() noexcept = default;

    /// Value Constructor.
    /// @param hull         Hull of this PolyBezier.
    /// @throws ValueError  If the number of points in the hull is not `(number of spline segments) * order + 1`
    PolyBezier(hull_t hull) : m_hull(std::move(hull)) {
        if ((!m_hull.is_empty()) && (m_hull.get_size() == 1 || (m_hull.get_size() - 1) % Order != 0)) {
            NOTF_THROW(ValueError,
                       "Number of elements for a Bezier spline of order {} must be `(n * {}) + 1` "
                       "with n = number of elements in the spline and n > 0",
                       Order, Order);
        }
    }

    /// Hull PolyLine of this PolyBezier.
    const hull_t& get_hull() const { return m_hull; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Hull of this PolyBezier.
    hull_t m_hull;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
