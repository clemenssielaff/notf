#pragma once

#include "notf/meta/hash.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// alignment ======================================================================================================== //

struct Alignment {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class for both Vertical- and Horizontal Alignments
    template<class Direction>
    struct _DirectedAlignment {

        // methods --------------------------------------------------------------------------------- //
    public:
        /// Value constructor. Produces a center alignment by default.
        /// @param value    Numeric alignment value in the range [0, 1].
        constexpr _DirectedAlignment(float value = 0.5) noexcept { set_value(value); }

        /// Equality operator.
        constexpr bool operator==(Direction other) noexcept { return is_approx(m_value, other.value); }

        /// Inequality operator.
        constexpr bool operator!=(Direction other) noexcept { return !is_approx(m_value, other.value); }

        /// Numeric value of this alignment in range [0, 1].
        constexpr float get_value() const noexcept { return m_value; }

        /// Changes the numeric value of the aligment.
        /// @param value    New value. The value is clamped into the range [0, 1]
        constexpr float set_value(float value) noexcept {
            m_value = clamp(value, 0, 1);
            return m_value;
        }

        // fields ---------------------------------------------------------------------------------- //
    private:
        /// Numeric alignment value
        float m_value;
    };

public:
    /// Relative horizontal alignment.
    struct Horizontal : public _DirectedAlignment<Horizontal> {

        /// Left alignment.
        constexpr static Horizontal left() { return {0.0}; }

        /// Center alignment.
        constexpr static Horizontal center() { return {0.5}; }

        /// Right alignment.
        constexpr static Horizontal right() { return {1.0}; }
    };

    /// Relative vertical alignment.
    struct Vertical : public _DirectedAlignment<Horizontal> {

        /// Bottom alignment.
        constexpr static Vertical bottom() { return {0.0}; }

        /// Center alignment.
        constexpr static Vertical center() { return {0.5}; }

        /// Top alignment.
        constexpr static Vertical top() { return {1.0}; }
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// @{
    /// Various convenience functions to create an Alignment.
    static Alignment bottom_left() { return {Vertical::bottom(), Horizontal::left()}; }
    static Alignment bottom_center() { return {Vertical::bottom(), Horizontal::center()}; }
    static Alignment bottom_right() { return {Vertical::bottom(), Horizontal::right()}; }

    static Alignment center_left() { return {Vertical::center(), Horizontal::left()}; }
    static Alignment center() { return {Vertical::center(), Horizontal::center()}; }
    static Alignment center_right() { return {Vertical::center(), Horizontal::right()}; }

    static Alignment top_left() { return {Vertical::top(), Horizontal::left()}; }
    static Alignment top_center() { return {Vertical::top(), Horizontal::center()}; }
    static Alignment top_right() { return {Vertical::top(), Horizontal::right()}; }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Vertical alignment
    Vertical vertical;

    /// Horizontal alignment
    Horizontal horizontal;
};

NOTF_CLOSE_NAMESPACE
