#pragma once

#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/polyline.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// polybezier ======================================================================================================= //

/// Polybeziers store the vertices in a Polyline with with the 1st, 4th, 7th ... vertex used both as the start of the
/// following bezier as well as the end of the previous one. Having the hull stored explicitly as a Polyline allows for
/// quick hull operations. However, this also means that in order to interpolate along the PolyBezier, it is more
/// efficient to construct a ParametricBezier for each segment and then re-use that instead of interpolating the
/// PolyBezier itself.
template<size_t Order, class Vector>
class PolyBezier {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type used to define the position of a point.
    using vector_t = Vector;

    /// Element type.
    using element_t = typename vector_t::element_t;

    /// Bezier type used for interpolation.
    using bezier_t = Bezier<Order, element_t>;

    /// Parametric Bezier type.
    using parabezier_t = ParametricBezier<Order, Vector2<element_t>>;

    /// Internal hull type.
    using hull_t = Polyline<element_t>;

private:
    /// Data held by a Bezier.
    using weights_t = typename bezier_t::weights_t;

    /// To differentiate the private from the public constructor taking a Polyline.
    struct Token {};

    // methods --------------------------------------------------------------------------------- //
private:
    /// Value Constructor.
    /// @param hull         Hull of this PolyBezier.
    /// @param token        To differentiate the private from the public constructor taking a Polyline.
    /// @throws ValueError  If the number of points in the hull is not `(number of spline segments) * order + 1`
    PolyBezier(Polyline<element_t> hull, Token) : m_hull(std::move(hull)) {
        if (!_has_valid_vertex_count()) {
            if (is_closed()) {
                NOTF_THROW(ValueError,
                           "Invalid vertex count: {0} - "
                           "The number of vertices (v) for a closed PolyBezier of order {1} must be "
                           "`(n * {1}) + 1` where (n) is the number of segments",
                           m_hull.get_vertex_count(), Order);
            } else {
                NOTF_THROW(ValueError,
                           "Invalid vertex count: {0} - "
                           "The number of vertices (v) for an open PolyBezier of order {1} must be "
                           "`n * {1}` where (n) is the number of segments",
                           m_hull.get_vertex_count(), Order);
            }
        }
    }

    /// Constructs the hull of a PolyBezier that is made up of only straight lines.
    /// @param polyline Polyline to emulate.
    static Polyline<element_t> _polyline_to_hull(Polyline<element_t>&& polyline) {
        if (polyline.is_empty()) { return polyline; }
        polyline.optimize();

        Polyline<element_t> result;
        result.set_closed(polyline.is_closed());
        const auto& input = polyline.get_vertices();
        auto& output = result.get_vertices();

        const size_t expected_size = 1 + Order * (input.size() - 1) + (polyline.is_closed() ? Order - 1 : 0);
        output.reserve(expected_size);

        // first - n-1 segment
        for (size_t i = 1; i < input.size(); ++i) {
            const auto delta = input[i] - input[i - 1];
            for (size_t step = 0; step < Order; ++step) {
                output.emplace_back((static_cast<element_t>(step) / static_cast<element_t>(Order)) * delta
                                    + input[i - 1]);
            }
        }

        // last vertex / segment
        if (polyline.is_closed()) {
            const auto delta = input.front() - input.back();
            for (size_t step = 0; step < Order; ++step) {
                output.emplace_back((static_cast<element_t>(step) / static_cast<element_t>(Order)) * delta
                                    + input.back());
            }
        } else {
            output.emplace_back(input.back());
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

    /// Whether or not this Polybezier is closed.
    bool is_closed() const { return m_hull.is_closed(); }

    /// Hull PolyLine of this PolyBezier.
    const Polyline<element_t>& get_hull() const { return m_hull; }

    /// Number of vertices in the hull of this PolyBezier.
    size_t get_vertex_count() const { return m_hull.get_vertex_count(); }

    /// Number of bezier segments in this Polybezier.
    size_t get_segment_count() const {
        NOTF_ASSERT(_has_valid_vertex_count());
        const size_t vertex_count = m_hull.get_vertices().size();
        if (is_closed()) {
            if (vertex_count < Order * 2) { return 0; } // closed Polybeziers need at least two segments
            return vertex_count / Order;
        } else {
            if (vertex_count < Order + 1) { return 0; }
            return (vertex_count - 1) / Order;
        }
    }

    /// The given segment of this PolyBezier.
    parabezier_t get_segment(const size_t index) const {
        const auto& vertices = m_hull.get_vertices();
        const size_t start_index = index * Order;
        const size_t wrap_index = vertices.size();
        if (start_index + Order > wrap_index - (is_closed() ? 0 : 1)) {
            NOTF_THROW(LogicError, "Cannot get Bezier segment {} from a PolyBezier with only {} segments", index,
                       get_segment_count());
        }
        weights_t x, y;
        for (size_t i = 0; i <= Order; ++i) {
            x[i] = vertices[(start_index + i) % wrap_index].x();
            y[i] = vertices[(start_index + i) % wrap_index].y();
        }
        return typename parabezier_t::data_t{bezier_t(std::move(x)), bezier_t(std::move(y))};
    }

    /// Position of a vector on the PolyBezier.
    /// The `t` argument is clamped to [0, n+1] for open hulls and [-(n+1), n+1] for closed ones, with n == number
    /// of bezier segments. If the hull is empty, the zero vector is returned.
    /// @param t    Position on the spline, integral part identifies the spline segment, fractional part the
    ///             position on that spline.
    vector_t interpolate(element_t t) const {
        if (is_empty()) { return vector_t::zero(); }

        { // normalize t
            const element_t n_plus_one = get_segment_count() + 1;
            t = clamp(t, (is_closed() ? -n_plus_one : 0), n_plus_one);
            if (t < 0) { t += n_plus_one; }
        }

        const auto& vertices = m_hull.get_vertices();

        // get the start index and the fraction of t to evaluate the spline
        size_t start_index;
        {
            const auto int_frac_pair = break_real<size_t>(t);
            start_index = int_frac_pair.first * Order;
            t = int_frac_pair.second;
            NOTF_ASSERT(start_index + Order < get_vertex_count());
        }

        // if t is close enough to a vertex, just return that instead
        if (t < precision_low<float>()) { return vertices[start_index]; }
        if (1 - t < precision_low<float>()) { return vertices[start_index + Order]; }

        // interpolate beziers
        weights_t x, y;
        for (size_t i = 0; i <= Order; ++i) {
            x[i] = vertices[start_index + i].x();
            y[i] = vertices[start_index + i].y();
        }
        return vector_t(bezier_t(std::move(x)).interpolate(t), bezier_t(std::move(y)).interpolate(t));
    }

private:
    bool _has_valid_vertex_count() const {
        const size_t vertex_count = m_hull.get_vertex_count();
        if (vertex_count < 2) { return true; }
        if (is_closed()) {
            return vertex_count % Order == 0;
        } else {
            return (vertex_count - 1) % Order == 0;
        }
    }

    // fields ---------------------------------------------------------------------------------- //
public: // TODO: clearly this shouldn't be public, but we need a way to get a hull from a polybezier and turn it back
        // into a Polybezier without going through _polyline_to_hull
    /// Hull of this PolyBezier.
    Polyline<element_t> m_hull;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
