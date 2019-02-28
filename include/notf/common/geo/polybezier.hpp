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

private:
    /// To differentiate the private from the public constructor taking a Polyline.
    struct Token {};

    // methods --------------------------------------------------------------------------------- //
private:
    /// Value Constructor.
    /// @param hull         Hull of this PolyBezier.
    /// @param token        To differentiate the private from the public constructor taking a Polyline.
    /// @throws ValueError  If the number of points in the hull is not `(number of spline segments) * order + 1`
    PolyBezier(hull_t hull, Token) : m_hull(std::move(hull)) {
        if ((!m_hull.is_empty()) && (m_hull.get_size() == 1 || (m_hull.get_size() - 1) % Order != 0)) {
            NOTF_THROW(ValueError,
                       "Number of elements for a Bezier spline of order {} must be `(n * {}) + 1` "
                       "with n = number of elements in the spline and n > 0",
                       Order, Order);
        }
    }

    /// Constructs the hull of a PolyBezier that is made up of only straight lines.
    /// @param polyline Polyline to emulate.
    static hull_t _polyline_to_hull(hull_t&& polyline) {
        if (polyline.is_empty()) { return polyline; }
        polyline.optimize();
        const auto& input = polyline.get_vertices();

        hull_t result;
        auto& output = result.get_vertices();
        output.reserve(1 + Order * (input.size() - 1));
        output.emplace_back(input[0]);
        for (size_t i = 1; i < input.size(); ++i) {
            output.emplace_back(input[i - 1]);
            output.emplace_back(input[i]);
            output.emplace_back(input[i]);
        }
        return result;
    }

public:
    /// Default constructor.
    constexpr PolyBezier() noexcept = default;

    /// Value Constructor.
    /// @param polyline     Polyline constructing a Polybezier of only straight lines..
    PolyBezier(Polyline<element_t> polyline) : PolyBezier(_polyline_to_hull(std::move(polyline)), Token{}) {}

    /// Checks whether the Polybezier has any vertices or not.
    bool is_empty() const { return m_hull.is_empty(); }

    /// Hull PolyLine of this PolyBezier.
    const hull_t& get_hull() const { return m_hull; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Hull of this PolyBezier.
    hull_t m_hull;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
