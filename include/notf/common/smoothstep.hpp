#pragma once

#include "notf/common/geo/vector2.hpp"

NOTF_OPEN_NAMESPACE

// smoothstep ======================================================================================================= //

namespace detail {

/// Custom smoothstep calculator.
/// Represents a cubic bezier curve with start at (0,0) and end at (1,1). The two control points are what determines the
/// shape of the smoothstep, they are only constrained insofar as the first (left) control point can never be right of
/// the second control point.
/// This could be a free function as well, but I figured that it will be a lot more common to set the control points
/// once and sampled many times. This way we don't have to enforce the ordering constraint of the control points with
/// every sample.
template<class Element>
class Smoothstep {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Scalar type used by this Smoothstep.
    using element_t = Element;

    /// Vector type used for the two control points.
    using vector_t = Vector2<element_t>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    /// Produces a steep sigmoid curve.
    constexpr Smoothstep() noexcept = default;

    /// Linear interpolation.
    constexpr static Smoothstep linear() noexcept {
        Smoothstep result;
        result.m_left_ctrl = vector_t(0, 0);
        result.m_right_ctrl = vector_t(1, 1);
        return result;
    }

    constexpr void set_left(const vector_t& left) noexcept {
        m_left_ctrl = {clamp(left.x()), clamp(left.y())};
        m_right_ctrl.x() = clamp(m_right_ctrl.x(), m_left_ctrl.x(), 1);
    }

    constexpr void set_right(const vector_t& right) noexcept {
        m_right_ctrl = {clamp(right.x()), clamp(right.y())};
        m_left_ctrl.x() = clamp(m_left_ctrl.x(), 0, m_right_ctrl.x());
    }

    constexpr vector_t get(const element_t t) noexcept {
        const element_t t2 = t * t;
        const element_t t3 = t2 * t;
        const element_t a = t * 3 - t2 * 6 + t3 * 3;
        const element_t b = t2 * 3 - t3 * 3;
        return {
            a * m_left_ctrl.x() + b * m_right_ctrl.x() + t3,
            a * m_left_ctrl.y() + b * m_right_ctrl.y() + t3,
        };
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    vector_t m_left_ctrl = vector_t(.5, 0);

    vector_t m_right_ctrl = vector_t(.5, 1);
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
