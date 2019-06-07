#pragma once

#include <vector>

#include "notf/meta/hash.hpp"
#include "notf/meta/integer.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// bezier =========================================================================================================== //

/// 1 dimensional Bezier segment.
/// Used as a building block for the PolyBezier spline.
template<size_t Order, class Element>
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
    static constexpr size_t get_order() noexcept { return Order; }

private:
    /// Actual bezier interpolation.
    /// Might look a bit overcomplicated, but produces the same assembly code for a bezier<3> as the
    /// "extremely-optimized version" from https://pomax.github.io/bezierinfo/#control
    /// (see https://godbolt.org/z/HbOyKJ for a comparison)
    template<size_t... I>
    constexpr auto _interpolate_impl(const element_t t, std::index_sequence<I...>) const {
        constexpr const std::array<element_t, Order + 1> binomials = pascal_triangle_row<Order, element_t>();
        const std::array<element_t, Order + 1> ts = power_list<Order + 1>(t);
        const std::array<element_t, Order + 1> its = power_list<Order + 1>(1 - t);
        auto lambda = [&](const ulong i) { return binomials[i] * m_weights[i] * ts[i] * its[Order - i]; };
        return sum(lambda(I)...);
    }

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Bezier() noexcept = default;

    /// Value constructor.
    /// @param data Data making up the Bezier.
    constexpr Bezier(weights_t data) noexcept : m_weights(std::move(data)) {}

    /// Value constructor.
    /// @param start    Start weight point of the Bezier, determines the data type.
    /// @param tail     All remaining weights.
    template<class T, class... Ts,
             class = std::enable_if_t<all(sizeof...(Ts) == Order,  //
                                          std::is_arithmetic_v<T>, //
                                          all_convertible_to_v<T, Ts...>)>>
    constexpr Bezier(T start, Ts... tail) : m_weights({start, tail...}) {}

    /// Straight line with constant interpolation speed.
    /// @param start    Start weight.
    /// @param end      End weight.
    constexpr static Bezier line(const element_t& start, const element_t& end) {
        const element_t delta = end - start;
        weights_t data{};
        for (size_t i = 0; i < Order + 1; ++i) {
            data[i] = (element_t(i) / element_t(Order)) * delta;
        }
        return Bezier(std::move(data));
    }

    /// Access to a single weight of this Bezier.
    /// @param index    Index of the weight. Must be in rande [0, Order].
    constexpr element_t get_weight(const size_t index) const {
        if (index > Order) { NOTF_THROW(IndexError, "Cannot get weight {} from Bezier of Order {}", index, Order); }
        return m_weights[index];
    }

    /// Bezier interpolation at position `t`.
    /// A bezier is most useful in [0, 1] but may be sampled outside that interval as well.
    /// @param t    Position to evaluate.
    constexpr element_t interpolate(const element_t& t) const {
        return _interpolate_impl(t, std::make_index_sequence<Order + 1>{});
    }

    /// The derivate bezier, can be used to calculate the tangent.
    constexpr std::enable_if_t<(Order > 0), Bezier<Order - 1, element_t>> get_derivate() const {
        std::array<element_t, Order> deriv_weights{};
        for (ulong k = 0; k < Order; ++k) {
            deriv_weights[k] = Order * (deriv_weights[k + 1] - deriv_weights[k]);
        }
        return deriv_weights;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Bezier weights.
    weights_t m_weights;
};

// parametric bezier ================================================================================================ //

/// Single Bezier segment with multidimensional data.
template<size_t Order, class Vector>
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
    static constexpr size_t get_order() noexcept { return Order; }

    /// Number of dimensions.
    static constexpr size_t get_dimensions() noexcept { return vector_t::get_dimensions(); }

private:
    /// Transforms individual vertices into an array of 1D beziers suitable for use in a Parametric Bezier.
    static constexpr data_t _deinterleave(std::array<vector_t, Order + 1>&& input) noexcept {
        std::array<typename bezier_t::weights_t, vector_t::get_dimensions()> output{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            for (uint i = 0; i < Order + 1; ++i) {
                output[dim][i] = input[i][dim];
            }
        }
        data_t result{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
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
    constexpr ParametricBezier(data_t data) noexcept : m_data(std::move(data)) {}

    /// Value constructor.
    /// @param vertices Individual vertices that make up this ParametricBezier.
    template<class... Ts,
             class = std::enable_if_t<all(sizeof...(Ts) == Order + 1, all_convertible_to_v<vector_t, Ts...>)>>
    constexpr ParametricBezier(Ts&&... vertices) noexcept
        : ParametricBezier(_deinterleave({std::forward<Ts>(vertices)...})) {}

    /// Access to a 1D Bezier that makes up this ParametricBezier.
    const bezier_t& get_dimension(const size_t dim) const {
        if (dim >= m_data.size()) {
            NOTF_THROW(IndexError, "Cannot get dimension {} from a ParametricBezier with only {} dimensions", dim,
                       get_dimensions());
        }
        return m_data[dim];
    }

    /// Access to a vertex of this Bezier.
    /// @param index    Index of the vertex. Must be in rande [0, Order].
    constexpr vector_t get_vertex(const size_t index) const {
        if (index > Order) { NOTF_THROW(IndexError, "Cannot get index {} from Bezier of Order {}", index, Order); }
        vector_t result{};
        for (size_t dim = 0; dim < get_dimensions(); ++dim) {
            result[dim] = m_data[dim].get_weight(index);
        }
        return result;
    }

    /// Bezier interpolation at position `t`.
    /// A bezier is most useful in [0, 1] but may be sampled outside that interval as well.
    /// @param t    Position to evaluate.
    constexpr vector_t interpolate(const element_t& t) const {
        vector_t result{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            result[dim] = m_data[0].interpolate(t);
        }
        return result;
    }

    /// The derivate bezier, can be used to calculate the tangent.
    constexpr std::enable_if_t<(Order > 0), ParametricBezier<Order - 1, vector_t>> get_derivate() const {
        typename ParametricBezier<Order - 1, vector_t>::data_t derivate{};
        for (uint dim = 0; dim < get_dimensions(); ++dim) {
            derivate[dim] = m_data[0].get_derivate();
        }
        return derivate;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Bezier weights.
    data_t m_data;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

/// std::hash specialization for Bezier.
template<size_t Order, class Element>
struct std::hash<notf::detail::Bezier<Order, Element>> {
    using type = notf::detail::Bezier<Order, Element>;
    size_t operator()(const type& bezier) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::BEZIER), bezier.order(), bezier.start.hash(),
                          bezier.ctrl1.hash(), bezier.ctrl2.hash(), bezier.end.hash());
    }
};

/// std::hash specialization for ParametricBezier.
template<size_t Order, class Vector>
struct std::hash<notf::detail::ParametricBezier<Order, Vector>> {
    using type = notf::detail::ParametricBezier<Order, Vector>;
    size_t operator()(const type& bezier) const {
        size_t result = ::notf::detail::versioned_base_hash();
        for (size_t i = 0; i < type::get_dimensions(); ++i) {
            ::notf::hash_combine(result, bezier.get_dimension(i));
        }
        return result;
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::CubicBezierf) == sizeof(float) * 4);
static_assert(notf::is_pod_v<notf::CubicBezierf>);
