#pragma once

#include <vector>

#include "notf/meta/allocator.hpp"
#include "notf/meta/hash.hpp"
#include "notf/meta/integer.hpp"

#include "notf/common/geo/polygon2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// bezier =========================================================================================================== //

/// 1 dimensional Bezier segment.
/// Used as a building block for the ParametricBezier.
template<std::size_t Order, class Element>
class Bezier {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Element type.
    using element_t = Element;

    /// Data held by this Bezier segment.
    using weights_t = std::array<element_t, Order + 1>;

    // helper ---------------------------------------------------------------------------------- //
public:
    /// Order of this Bezier spline.
    static constexpr std::size_t get_order() noexcept { return Order; }

private:
    /// Actual bezier interpolation.
    /// Might look a bit overcomplicated, but produces the same assembly code for a Bezier<3> as the
    /// "extremely-optimized version" from https://pomax.github.io/bezierinfo/#control
    /// Edit: ... at least for clang9.
    /// (see https://godbolt.org/z/HbOyKJ for a comparison)
    template<std::size_t... I>
    constexpr auto _interpolate_impl(const element_t t, std::index_sequence<I...>) const {
        constexpr const std::array<element_t, Order + 1> binomials = pascal_triangle_row<Order, element_t>();
        const std::array<element_t, Order + 1> ts = power_list<Order + 1>(t);
        const std::array<element_t, Order + 1> its = power_list<Order + 1>(1 - t);
        auto lambda = [&](const ulong i) { return binomials[i] * weights[i] * ts[i] * its[Order - i]; };
        return sum(lambda(I)...);
    }

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Bezier() noexcept = default;

    /// Value constructor.
    /// @param data Data making up the Bezier.
    constexpr Bezier(weights_t data) noexcept : weights(std::move(data)) {}

    /// Value constructor.
    /// @param start    Start weight point of the Bezier, determines the data type.
    /// @param tail     All remaining weights.
    template<class T, class... Ts,
             class = std::enable_if_t<all(sizeof...(Ts) == Order,  //
                                          std::is_arithmetic_v<T>, //
                                          all_convertible_to_v<T, Ts...>)>>
    constexpr Bezier(T start, Ts... tail) : weights({start, tail...}) {}

    /// Straight line with constant interpolation speed.
    /// @param start    Start weight.
    /// @param end      End weight.
    constexpr static Bezier line(const element_t& start, const element_t& end) {
        const element_t delta = end - start;
        weights_t data{};
        for (uint i = 0; i < Order + 1; ++i) {
            data[i] = (element_t(i) / element_t(Order)) * delta;
        }
        return Bezier(std::move(data));
    }

    /// Access to a single weight of this Bezier.
    /// @param index    Index of the weight. Must be in rande [0, Order].
    constexpr element_t get_weight(const std::size_t index) const {
        if (index > Order) { NOTF_THROW(IndexError, "Cannot get weight {} from Bezier of Order {}", index, Order); }
        return weights[index];
    }

    /// Bezier interpolation at position `t`.
    /// A Bezier is most useful in [0, 1] but may be sampled outside that interval as well.
    /// @param t    Position to evaluate.
    constexpr element_t interpolate(const element_t& t) const {
        return _interpolate_impl(t, std::make_index_sequence<Order + 1>{});
    }

    /// The derivate Bezier, can be used to calculate the tangent.
    constexpr std::enable_if_t<(Order > 0), Bezier<Order - 1, element_t>> get_derivate() const {
        std::array<element_t, Order> deriv_weights{};
        for (uint k = 0; k < Order; ++k) {
            deriv_weights[k] = Order * (deriv_weights[k + 1] - deriv_weights[k]);
        }
        return deriv_weights;
    }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const Bezier& other) const noexcept { return weights == other.weights; }

    /// Inequality operator.
    /// @param other    Value to test against.
    constexpr bool operator!=(const Bezier& other) const noexcept { return weights != other.weights; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Bezier weights.
    weights_t weights;
};

// parametric bezier ================================================================================================ //

/// Single Bezier segment with multidimensional data.
template<std::size_t Order, class Vector>
class ParametricBezier {

    // types ----------------------------------------------------------------------------------- //
public:
    using vector_t = Vector;

    /// Element type.
    using element_t = typename vector_t::element_t;

    /// Single Bezier.
    using bezier_t = Bezier<Order, element_t>;

    /// Multidimensional Bezier array.
    using data_t = std::array<bezier_t, vector_t::get_dimensions()>;

    // helper ---------------------------------------------------------------------------------- //
public:
    /// Order of this Bezier spline.
    static constexpr std::size_t get_order() noexcept { return Order; }

    /// Number of dimensions.
    static constexpr std::size_t get_dimensions() noexcept { return vector_t::get_dimensions(); }

private:
    /// Transforms individual vertices into an array of 1D beziers suitable for use in a Parametric Bezier.
    static constexpr data_t _deinterleave(std::array<vector_t, Order + 1>&& input) noexcept {
        std::array<typename bezier_t::weights_t, vector_t::get_dimensions()> output{};
        for (std::size_t dim = 0; dim < get_dimensions(); ++dim) {
            for (std::size_t i = 0; i < Order + 1; ++i) {
                output[dim][i] = input[i][dim];
            }
        }
        data_t result{};
        for (std::size_t dim = 0; dim < get_dimensions(); ++dim) {
            result[dim] = std::move(output[dim]);
        }
        return result;
    }

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr ParametricBezier() noexcept = default;

    /// Value constructor.
    /// @param data Data making up the Bezier.
    constexpr ParametricBezier(data_t data) noexcept : data(std::move(data)) {}

    /// Value constructor.
    /// @param vertices Individual vertices that make up this ParametricBezier.
    template<class... Ts,
             class = std::enable_if_t<all(sizeof...(Ts) == Order + 1, all_convertible_to_v<vector_t, Ts...>)>>
    constexpr ParametricBezier(Ts&&... vertices) noexcept
        : ParametricBezier(_deinterleave({std::forward<Ts>(vertices)...})) {}

    /// Access to a 1D Bezier that makes up this ParametricBezier.
    const bezier_t& get_dimension(const std::size_t dim) const {
        if (dim >= data.size()) {
            NOTF_THROW(IndexError, "Cannot get dimension {} from a ParametricBezier with only {} dimensions", dim,
                       get_dimensions());
        }
        return data[dim];
    }

    /// Access to a vertex of this Bezier.
    /// @param index    Index of the vertex. Must be in range [0, Order].
    constexpr vector_t get_vertex(const uint index) const {
        if (index > Order) {
            NOTF_THROW(IndexError, "Cannot get index {} from Bezier of Order {}", index, Order);
        } else {
            return _get_vertex(index);
        }
    }

    /// All vertices in this Bezier.
    constexpr std::array<vector_t, Order + 1> get_vertices() const noexcept {
        std::array<vector_t, Order + 1> result{};
        for (uint index = 0; index <= get_order(); ++index) {
            result[index] = _get_vertex(index);
        }
        return result;
    }

    /// Bezier interpolation at position `t`.
    /// A Bezier is most useful in [0, 1] but may be sampled outside that interval as well.
    /// @param t    Position to evaluate.
    constexpr vector_t interpolate(const element_t& t) const {
        vector_t result{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            result[dim] = data[dim].interpolate(t);
        }
        return result;
    }

    /// The derivate Bezier, can be used to calculate the tangent.
    constexpr std::enable_if_t<(Order > 0), ParametricBezier<Order - 1, vector_t>> get_derivate() const {
        typename ParametricBezier<Order - 1, vector_t>::data_t derivate{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            derivate[dim] = data[dim].get_derivate();
        }
        return derivate;
    }

    /// Get the number of line segments required to approximate the bezier within a given tolerance.
    ///
    /// This method works on the difference between the interpolated point on the bezier and the linearly interpolated
    /// point on the straight line between the start and end.
    /// The means that a bezier that looks like a straight line will still need multiple segments if its interpolation
    /// "speed" is not linear.
    ///
    /// From: https://raphlinus.github.io/graphics/curves/2019/12/23/flatten-quadbez.html
    /// where it is presented as an alternative approach, originally from Sederberg's CAGD notes:
    /// https://scholarsarchive.byu.edu/cgi/viewcontent.cgi?article=1000&context=facpub#section.10.6
    ///
    /// @param tolerance (Default = 1)  Maximum distance from a line segment to the bezier spline.
    /// @returns         Number of segments.
    constexpr uint get_segment_count(const element_t tolerance = 1) const noexcept {
        std::array<vector_t, Order + 1> vertices;
        for (uint i = 0; i <= Order; ++i) {
            vertices[i] = get_vertex(i);
        }
        static_assert(Order >= 2);
        vector_t bounds = vertices[2] - (2 * vertices[1]) + vertices[0];
        if constexpr (Order > 2) { bounds.set_abs(); }
        for (uint i = 1; i + 2 <= Order; ++i) {
            bounds.set_max((vertices[i + 2] - (2 * vertices[i + 1]) + vertices[i]).get_abs());
        }
        return static_cast<uint>(
            element_t(1) + notf::sqrt(((Order * (Order - 1)) * bounds).get_magnitude() / (element_t(8) * tolerance)));
    }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const ParametricBezier& other) const noexcept { return data == other.data; }

    /// Inequality operator.
    /// @param other    Value to test against.
    constexpr bool operator!=(const ParametricBezier& other) const noexcept { return data != other.data; }

private:
    constexpr vector_t _get_vertex(const uint index) const noexcept {
        assert(index <= Order);
        vector_t result{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            result[dim] = data[dim].get_weight(index);
        }
        return result;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Bezier weights.
    data_t data;
};

} // namespace detail

// utility functions ================================================================================================ //

/// Approximates the given Bezier with a polygon.
/// @param tolerance (Default = 1)  Maximum distance from a line segment to the bezier spline.
template<std::size_t Order, class Element>
detail::Polygon2<Element> approximate(const detail::ParametricBezier<Order, detail::Vector2<Element>>& bezier,
                                      const Element tolerance = 1) noexcept {
    using polygon_t = detail::Polygon2<Element>;
    using vertex_t = typename polygon_t::vertex_t;
    const uint segment_count = bezier.get_segment_count(tolerance);
    std::vector<vertex_t> vertices(segment_count + 1, DefaultInitAllocator<vertex_t>());
    vertices[0] = bezier.get_vertex(0);
    for (uint i = 1; i < segment_count; ++i) {
        vertices[i] = bezier.interpolate(Element(i) / Element(segment_count));
    }
    vertices[segment_count] = bezier.get_vertex(bezier.get_order());
    return polygon_t(std::move(vertices));
}

/*
def cubic_split_middle(cubic_bezier: CubicBezier2f) -> Tuple[CubicBezier2f, CubicBezier2f]:
    """
    From: http://www.timotheegroleau.com/Flash/articles/cubic_bezier_in_flash.htm
    """
    v: List[V2f] = cubic_bezier.get_vertices()
    a1: V2f = (v[0] + v[1]) / 2
    a2: V2f = (v[1] + v[2]) / 2
    a3: V2f = (v[2] + v[3]) / 2
    b1: V2f = (a1 + a2) / 2
    b2: V2f = (a2 + a3) / 2
    b3: V2f = (b1 + b2) / 2
    return (
        CubicBezier2f(v[0], a1, b1, b3),
        CubicBezier2f(b3, b2, a3, v[3]),
    )


def cubic_split(bezier: CubicBezier2f, t: float) -> Tuple[CubicBezier2f, CubicBezier2f]:
    """
    Split a cubic Bezier into two new cubic Bezier curves at a given point t.
    See https://math.stackexchange.com/questions/877725
    """
    t = max(0., min(1., t))
    u: float = 1 - t
    start, ctrl1, ctrl2, end = bezier.get_vertices()

    temp: V2f = ctrl1 * u + ctrl2 * t
    first_ctrl1: V2f = start * u + ctrl1 * t
    second_ctrl2: V2f = ctrl2 * u + end * t
    first_ctrl2: V2f = first_ctrl1 * u + temp * t
    second_ctrl1: V2f = temp * u + second_ctrl2 * t
    mid: V2f = first_ctrl2 * u + second_ctrl1 * t
    return (
        CubicBezier2f(start, first_ctrl1, first_ctrl2, mid),
        CubicBezier2f(mid, second_ctrl1, second_ctrl2, end),
    )


def cubic_inflections(vertices: List[V2f]) -> Tuple[Optional[float], Optional[float]]:
    """
    Inflections points (in t) of a cubic bezier spline.
    A cubic bezier can have up to 2 inflection points.
    From https://pomax.github.io/bezierinfo/#inflections
    """
    aligned: List[V2f] = align_bezier(vertices)
    a: float = aligned[2].x * aligned[1].y
    b: float = aligned[3].x * aligned[1].y
    c: float = aligned[1].x * aligned[2].y
    d: float = aligned[3].x * aligned[2].y
    x: float = 18 * (-3 * a + 2 * b + 3 * c - d)
    y: float = 18 * (3 * a - b - 3 * c)
    z: float = 18 * (c - a)
    if is_approx(x, 0):
        if not is_approx(y, 0):
            t: float = -z / y
            if 0 <= t <= 1:
                return t, None
        return None, None
    else:
        root_term: float = math.sqrt(y * y - 4 * x * z)
        first: float = (root_term - y) / (2 * x)
        second: float = -(root_term + y) / (2 * x)
        return (
            first if 0 <= first <= 1 else None,
            second if 0 <= second <= 1 else None,
        )


def approximate_cubic_one(cubic_bezier: CubicBezier2f) -> SquareBezier2f:
    """
    Best approximation of a cubic bezier with a single square bezier.
    From https://groups.google.com/d/msg/comp.fonts/E7QpciSt9KQ/aG_E-I9ZoRMJ
    """
    start, ctrl1, ctrl2, end = cubic_bezier.get_vertices()
    return SquareBezier2f(
        start, (-start + 3 * ctrl1 + 3 * ctrl2 - end) / 4, end
    )


def approximate_cubic_four(cubic_bezier: CubicBezier2f) -> \
        Tuple[SquareBezier2f, SquareBezier2f, SquareBezier2f, SquareBezier2f]:
    """
    Quick approximation of a cubic bezier with four square bezier splines.
    From: https://timotheegroleau.com/Flash/articles/cubic_bezier_in_flash.htm
    """
    start, ctrl1, ctrl2, end = cubic_bezier.get_vertices()

    t1: V2f = lerp(start, ctrl1, 3 / 4)
    t2: V2f = lerp(end, ctrl2, 3 / 4)
    sixteenth_delta: V2f = (end - start) / 16

    first_ctrl = lerp(start, ctrl1, 3 / 8)
    second_ctrl = lerp(t1, t2, 3 / 8) - sixteenth_delta
    third_ctrl = lerp(t2, t1, 3 / 8) + sixteenth_delta
    forth_ctrl = lerp(end, ctrl2, 3 / 8)

    first_anchor: V2f = (first_ctrl + second_ctrl) / 2
    second_anchor: V2f = (t1 + t2) / 2
    third_anchor: V2f = (third_ctrl + forth_ctrl) / 2

    return (
        SquareBezier2f(start, first_ctrl, first_anchor),
        SquareBezier2f(first_anchor, second_ctrl, second_anchor),
        SquareBezier2f(second_anchor, third_ctrl, third_anchor),
        SquareBezier2f(third_anchor, forth_ctrl, end),
    )
*/

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

/// std::hash specialization for Bezier.
template<std::size_t Order, class Element>
struct std::hash<notf::detail::Bezier<Order, Element>> {
    using type = notf::detail::Bezier<Order, Element>;
    std::size_t operator()(const type& bezier) const {
        return notf::hash(static_cast<std::size_t>(notf::detail::HashID::BEZIER), bezier.order(), bezier.start.hash(),
                          bezier.ctrl1.hash(), bezier.ctrl2.hash(), bezier.end.hash());
    }
};

/// std::hash specialization for ParametricBezier.
template<std::size_t Order, class Vector>
struct std::hash<notf::detail::ParametricBezier<Order, Vector>> {
    using type = notf::detail::ParametricBezier<Order, Vector>;
    std::size_t operator()(const type& bezier) const {
        std::size_t result = ::notf::detail::versioned_base_hash();
        for (std::size_t i = 0; i < type::get_dimensions(); ++i) {
            ::notf::hash_combine(result, bezier.get_dimension(i));
        }
        return result;
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::CubicBezierf) == sizeof(float) * 4);
static_assert(notf::is_pod_v<notf::CubicBezierf>);
