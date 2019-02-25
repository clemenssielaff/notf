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

private:
    /// Data held by this Bezier segment.
    using weights_t = std::array<element_t, Order + 1>;

    // helper ---------------------------------------------------------------------------------- //
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

public:
    /// Default constructor.
    constexpr Bezier() noexcept = default;

    /// Value constructor.
    /// @param data Data making up the Bezier.
    constexpr Bezier(const weights_t& data) noexcept : m_weights(data) {}

    /// Value constructor.
    /// @param start    Start weight point of the Bezier, determines the data type.
    /// @param tail     All remaining weights.
    template<class T, class... Ts,
             class = std::enable_if_t<all(sizeof...(Ts) == Order,  //
                                          std::is_arithmetic_v<T>, //
                                          all_convertible_to<T, Ts...>)>>
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

    /// Order of this Bezier spline.
    static constexpr size_t get_order() noexcept { return Order; }

    /// Bezier interpolation at position `t`.
    /// A bezier is most useful in [0, 1] but may be sampled outside that interval as well.
    /// @param t    Position to evaluate.
    constexpr element_t interpolate(const element_t& t) const {
        return _interpolate_impl(t, std::make_index_sequence<Order + 1>{});
    }

    /// The derivate bezier, can be used to calculate the tangent.
    constexpr std::enable_if_t<(Order > 0), Bezier<Order - 1, element_t>> get_derivate() const {
        std::array<element_t, Order - 1> deriv_weights;
        for (ulong k = 0; k < Order; ++k) {
            deriv_weights[k] = Order * (deriv_weights[k + 1] - deriv_weights[k]);
        }
        return Bezier<Order - 1, element_t>(std::move(deriv_weights));
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Bezier weights.
    weights_t m_weights;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

/// std::hash specialization for Bezier.
template<size_t ORDER, typename VECTOR>
struct std::hash<notf::detail::Bezier<ORDER, VECTOR>> {
    size_t operator()(const notf::detail::Bezier<ORDER, VECTOR>& bezier) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::BEZIER), bezier.order(), bezier.start.hash(),
                          bezier.ctrl1.hash(), bezier.ctrl2.hash(), bezier.end.hash());
    }
};

// compile time tests =============================================================================================== //

static_assert(sizeof(notf::CubicBezierf) == sizeof(float) * 4);
static_assert(notf::is_pod_v<notf::CubicBezierf>);
