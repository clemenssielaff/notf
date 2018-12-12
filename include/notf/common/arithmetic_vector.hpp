#pragma once

#include "notf/common/arithmetic.hpp"

NOTF_OPEN_NAMESPACE

// arithmetic vector ================================================================================================ //

namespace detail {

/// Arithmetic vector implementation.
template<class Derived, class Element, size_t Dimensions>
struct ArithmeticVector : public Arithmetic<Derived, Element, Element, Dimensions> {

    // types --------------------------------------------------------------------------------- //
public:
    using super_t = Arithmetic<Derived, Element, Element, Dimensions>;
    using derived_t = typename super_t::derived_t;
    using element_t = typename super_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    ArithmeticVector() = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr ArithmeticVector(Args&&... args) : super_t(std::forward<Args>(args)...) {}

    // magnitude --------------------------------------------------------------

    /// Check whether this vector is of unit magnitude.
    constexpr bool is_unit() const noexcept { return abs(get_magnitude_sq() - 1) <= precision_high<element_t>(); }

    /// Calculate the squared magnitude of this vector.
    constexpr element_t get_magnitude_sq() const noexcept {
        element_t result = 0;
        for (size_t i = 0; i < super_t::get_dimensions(); ++i) {
            result += data[i] * data[i];
        }
        return result;
    }

    /// Returns the magnitude of this vector.
    element_t get_magnitude() const noexcept { return sqrt(get_magnitude_sq()); }

    /// Normalizes this vector in-place.
    derived_t& normalize() {
        const element_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<element_t>()) { // is unit
            return *static_cast<derived_t*>(this);
        }
        if (abs(mag_sq) <= precision_high<element_t>()) { // is zero
            return *static_cast<derived_t*>(this);
        }
        return *this /= sqrt(mag_sq);
    }

    /// Normalizes this vector in-place.
    /// Is fast but imprecise.
    derived_t& fast_normalize() { return *this /= fast_inv_sqrt(get_magnitude_sq()); }

    /// Returns a normalized copy of this vector.
    derived_t get_normalized() const& {
        derived_t result = *this;
        result.normalize();
        return result;
    }
    derived_t&& get_normalized() && { return normalize(); }

    // geometric --------------------------------------------------------------

    /// Returns the dot product of this vector and another.
    /// @param other     Other vector.
    constexpr element_t dot(const derived_t& other) const noexcept { return (*this * other).sum(); }

    /// Tests whether this vector is orthogonal to the other.
    /// The zero vector is orthogonal to every other vector.
    /// @param other     Vector to test against.
    bool is_orthogonal_to(const derived_t& other) const {
        // normalization required for large absolute differences in vector lengths
        return abs(get_normalized().dot(other.get_normalized())) <= precision_high<element_t>();
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

// free functions =================================================================================================== //

/// Linear interpolation between two arithmetic values.
/// @param from    Left value, full weight at blend <= 0.
/// @param to      Right value, full weight at blend >= 1.
/// @param blend   Blend value, clamped to range [0, 1].
template<class Value, class = std::enable_if_t<detail::is_arithmetic_type<Value>>>
constexpr Value lerp(const Value& from, const Value& to, const typename Value::element_t blend) noexcept {
    if (blend <= 0) {
        return from;
    } else if (blend >= 1) {
        return to;
    }
    return ((to - from) *= blend) += from;
}

NOTF_CLOSE_NAMESPACE
