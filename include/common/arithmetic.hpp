#pragma once

#include <array>
#include <assert.h>

#include "common/hash.hpp"

namespace notf {
namespace detail {

//*********************************************************************************************************************/

/** Base class for all arithmetic value types.
 *
 * The Arithmetic base class provides naive implementations of each operation.
 * You can override specific functionality for value-specific behaviour or to make use of SIMD instructions.
 */
template <typename SPECIALIZATION, typename VALUE_TYPE, size_t DIMENSIONS, bool BASE_FOR_PARTIAL = false>
struct Arithmetic {

    /* Types and Fields ***********************************************************************************************/

    /** Value data type. */
    using value_t = VALUE_TYPE;

    /** Value data array. */
    std::array<value_t, DIMENSIONS> data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    Arithmetic() = default;

    /** Perfect forwarding constructor. */
    template <typename... T>
    Arithmetic(T&&... ts)
        : data{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Set all elements to zero. */
    static SPECIALIZATION zero() { return fill(0); }

    /** Set all elements to a given value. */
    static SPECIALIZATION fill(const value_t value)
    {
        SPECIALIZATION result;
        result.data.fill(value);
        return result;
    }

    /* Inspection  ****************************************************************************************************/

    /** Dimensions of the value. */
    static constexpr size_t size() { return DIMENSIONS; }

    /** Checks, if this value contains only real, finite values (no INFINITY or NAN). */
    bool is_real() const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (!notf::is_real(data[i])) {
                return false;
            }
        }
        return true;
    }

    /** Returns true if all elements are (approximately) zero.
     * @param epsilon   Largest difference that is still considered to be zero.
     */
    bool is_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (abs(data[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /** Checks, if any element of this value is (approximately) zero. */
    bool contains_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (abs(data[i]) <= epsilon) {
                return true;
            }
        }
        return false;
    }

    /** Component-wise check if this value is approximately the same as another.
     * @param other     Value to test against.
     * @param epsilon   Maximal allowed distance between the two values.
     */
    bool is_approx(const SPECIALIZATION& other, const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (abs(data[i] - other[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /** Checks whether this value is of unit magnitude. */
    bool is_unit() const { return abs(get_magnitude_sq() - 1) <= precision_high<value_t>(); }

    /** Calculates and returns the hash of this value. */
    size_t hash() const
    {
        std::size_t result = 0;
        for (size_t i = 0; i < size(); ++i) {
            notf::hash_combine(result, data[i]);
        }
        return result;
    }

    /** Checks whether two values are equal. */
    bool operator==(const SPECIALIZATION& other) const { return data == other.data; }

    /** Checks whether two values are not equal. */
    bool operator!=(const SPECIALIZATION& other) const { return data != other.data; }

    /** Read-only reference to an element of this value by index. */
    const value_t& operator[](const size_t index) const
    {
        assert(index < size());
        return data[index];
    }

    /** Read-only pointer to the value's internal storage. */
    const value_t* as_ptr() const { return &data[0]; }

    /** Returns the squared magnitude of this value. */
    value_t get_magnitude_sq() const
    {
        value_t result = 0;
        for (size_t i = 0; i < size(); ++i) {
            result += data[i] * data[i];
        }
        return result;
    }

    /** Returns the magnitude of this value. */
    value_t get_magnitude() const { return sqrt(get_magnitude_sq()); }

    /** Modification **************************************************************************************************/

    /** Read-write reference to an element of this value by index. */
    value_t& operator[](const size_t index)
    {
        assert(index < size());
        return data[index];
    }

    /** Read-write pointer to the value's internal storage. */
    value_t* as_ptr() { return &data[0]; }

    /** Set all elements of this value. */
    SPECIALIZATION& set_all(const value_t value)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] = value;
        }
        return _specialized_this();
    }

    /** Set all elements to zero. */
    SPECIALIZATION& set_zero() { return set_all(0); }

    /** The sum of this value with another one. */
    SPECIALIZATION operator+(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] += other[i];
        }
        return result;
    }

    /** In-place addition of another value. */
    SPECIALIZATION& operator+=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] += other[i];
        }
        return _specialized_this();
    }

    /** The difference between this value and another one. */
    SPECIALIZATION operator-(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] -= other[i];
        }
        return result;
    }

    /** In-place subtraction of another value. */
    SPECIALIZATION& operator-=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] -= other[i];
        }
        return _specialized_this();
    }

    /** Component-wise multiplication of this value with another. */
    SPECIALIZATION operator*(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] *= other[i];
        }
        return result;
    }

    /** In-place component-wise multiplication with another value. */
    SPECIALIZATION& operator*=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] *= other[i];
        }
        return _specialized_this();
    }

    /** Multiplication of this value with a scalar. */
    SPECIALIZATION operator*(const value_t factor) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] *= factor;
        }
        return result;
    }

    /** In-place multiplication with a scalar. */
    SPECIALIZATION& operator*=(const value_t factor)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] *= factor;
        }
        return _specialized_this();
    }

    /** Component-wise division of this value by another. */
    SPECIALIZATION operator/(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] /= other[i];
        }
        return result;
    }

    /** In-place component-wise division by another value. */
    SPECIALIZATION& operator/=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] /= other[i];
        }
        return _specialized_this();
    }

    /** Division of this value by a scalar. */
    SPECIALIZATION operator/(const value_t divisor) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < size(); ++i) {
            result[i] /= divisor;
        }
        return result;
    }

    /** In-place division by a scalar. */
    SPECIALIZATION& operator/=(const value_t divisor)
    {
        for (size_t i = 0; i < size(); ++i) {
            data[i] /= divisor;
        }
        return _specialized_this();
    }

    /** The inverted value. */
    SPECIALIZATION operator-() const { return *this * -1; }

    /** A normalized copy of this value. */
    SPECIALIZATION get_normal() const
    {
        const value_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<value_t>()) {
            SPECIALIZATION result;
            result.data = data;
            return result; // is unit
        }
        if (abs(mag_sq) <= precision_high<value_t>()) {
            return SPECIALIZATION::zero(); // is zero
        }
        return *this / sqrt(mag_sq);
    }

    /** In-place normalization of this value. */
    SPECIALIZATION& normalize()
    {
        const value_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<value_t>()) {
            return _specialized_this(); // is unit
        }
        if (abs(mag_sq) <= precision_high<value_t>()) {
            set_zero(); // is zero
            return _specialized_this();
        }
        return *this /= sqrt(mag_sq);
    }

protected: // methods
    constexpr SPECIALIZATION& _specialized_this() { return *reinterpret_cast<SPECIALIZATION*>(this); }
};

} // namespace detail

// Free Functions *****************************************************************************************************/

/** Linear interpolation between two values.
 * @param from    Left Vector, full weight at blend <= 0.
 * @param to      Right Vector, full weight at blend >= 1.
 * @param blend   Blend value, clamped to range [0, 1].
 */
template <typename SPECIALIZATION, typename VALUE_TYPE, size_t DIMENSIONS>
detail::Arithmetic<SPECIALIZATION, VALUE_TYPE, DIMENSIONS> lerp(
    const detail::Arithmetic<SPECIALIZATION, VALUE_TYPE, DIMENSIONS>& from,
    const detail::Arithmetic<SPECIALIZATION, VALUE_TYPE, DIMENSIONS>& to,
    const VALUE_TYPE blend)
{
    return ((to - from) *= clamp(blend, 0, 1)) += from;
}

} // namespace notf
