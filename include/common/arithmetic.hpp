#pragma once

#include <array>
#include <assert.h>
#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"

namespace notf {

namespace detail {

//*********************************************************************************************************************/

/** Lowest common denominator for all Value types. */
template <typename VALUE_TYPE, size_t DIMENSIONS>
struct Value {

    /** Data type of the Value. */
    using value_t = VALUE_TYPE;

    /** Dimensions of the value. */
    static constexpr size_t size() { return DIMENSIONS; }
};

//*********************************************************************************************************************/

/** Base class for all arithmetic value types.
 * `BASE` must provide a `array` field containing all elements of the value.
 *
 * The Arithmetic base class implements the most naive version of each operation.
 * You can override specific functionality for value-specific behaviour or to make use of SIMD instructions.
 */
template <typename SPECIALIZATION, typename VALUE_CLASS>
struct Arithmetic : public VALUE_CLASS {

    // explitic forwards
    using VALUE_CLASS::array;
    using VALUE_CLASS::size;

    /* Types **********************************************************************************************************/

    using value_t = typename VALUE_CLASS::value_t;

    /* Constructors ***************************************************************************************************/

    Arithmetic() = default;

    template <typename... T>
    Arithmetic(T&&... ts)
        : VALUE_CLASS{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Set all elements to zero. */
    static SPECIALIZATION zero() { return fill(0); }

    /** Set all elements to a given value. */
    static SPECIALIZATION fill(const value_t value)
    {
        SPECIALIZATION result;
        result.array.fill(value);
        return result;
    }

    /* Inspection  ****************************************************************************************************/

    /** Checks, if this value contains only real, finite values (no INFINITY or NAN). */
    bool is_real() const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (!notf::is_real(array[i])) {
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
            if (abs(array[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /** Checks, if any element of this value is (approximately) zero. */
    bool contains_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (abs(array[i]) <= epsilon) {
                return true;
            }
        }
        return false;
    }

    /** Component-wise check if this value is approximately the same as another.
     * @param other     Value to test against.
     * @param epsilon   Maximal allowed distance between the two values.
     */
    bool is_approx(const Arithmetic& other, const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (abs(array[i] - other[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /** Calculates and returns the hash of this value. */
    size_t hash() const
    {
        std::size_t result = 0;
        for (size_t i = 0; i < size(); ++i) {
            notf::hash_combine(result, array[i]);
        }
        return result;
    }

    /** Checks whether two values are equal. */
    bool operator==(const Arithmetic& other) const { return array == other.array; }

    /** Checks whether two values are not equal. */
    bool operator!=(const Arithmetic& other) const { return array != other.array; }

    /** Read-only reference to an element of this value by index. */
    const value_t& operator[](const size_t index) const
    {
        assert(index < size());
        return array[index];
    }

    /** Read-only pointer to the value's internal storage. */
    const value_t* as_ptr() const { return &array; }

    /** Modification **************************************************************************************************/

    /** Read-write reference to an element of this value by index. */
    value_t& operator[](const size_t index)
    {
        assert(index < size());
        return array[index];
    }

    /** Read-write pointer to the value's internal storage. */
    value_t* as_ptr() { return &array; }

    /** Set all elements of this value. */
    Arithmetic& set_all(const value_t value)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] = value;
        }
        return *this;
    }

    /** Set all elements to zero. */
    Arithmetic& set_zero() { return set_all(0); }

    /** The sum of this value with another one. */
    SPECIALIZATION operator+(const Arithmetic& other) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] += other[i];
        }
        return result;
    }

    /** In-place addition of another value. */
    Arithmetic& operator+=(const Arithmetic& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] += other[i];
        }
        return *this;
    }

    /** The difference between this value and another one. */
    SPECIALIZATION operator-(const Arithmetic& other) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] -= other[i];
        }
        return result;
    }

    /** In-place subtraction of another value. */
    Arithmetic& operator-=(const Arithmetic& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] -= other[i];
        }
        return *this;
    }

    /** Component-wise multiplication of this value with another. */
    SPECIALIZATION operator*(const Arithmetic& other) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] *= other[i];
        }
        return result;
    }

    /** In-place component-wise multiplication with another value. */
    Arithmetic& operator*=(const Arithmetic& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] *= other[i];
        }
        return *this;
    }

    /** Multiplication of this value with a scalar. */
    SPECIALIZATION operator*(const value_t factor) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] *= factor;
        }
        return result;
    }

    /** In-place multiplication with a scalar. */
    Arithmetic& operator*=(const value_t factor)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] *= factor;
        }
        return *this;
    }

    /** Component-wise division of this value by another. */
    SPECIALIZATION operator/(const Arithmetic& other) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] /= other[i];
        }
        return result;
    }

    /** In-place component-wise division by another value. */
    Arithmetic& operator/=(const Arithmetic& other)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] /= other[i];
        }
        return *this;
    }

    /** Division of this value by a scalar. */
    SPECIALIZATION operator/(const value_t divisor) const
    {
        SPECIALIZATION result;
        result.array = array;
        for (size_t i = 0; i < size(); ++i) {
            result[i] /= divisor;
        }
        return result;
    }

    /** In-place division by a scalar. */
    Arithmetic& operator/=(const value_t divisor)
    {
        for (size_t i = 0; i < size(); ++i) {
            array[i] /= divisor;
        }
        return *this;
    }

    /** The inverted value. */
    Arithmetic operator-() const { return *this * -1; }
};

} // namespace detail

/* Free Functions *****************************************************************************************************/

/** Linear interpolation between two Vector2s.
 * @param from    Left Vector, full weight at blend <= 0.
 * @param to      Right Vector, full weight at blend >= 1.
 * @param blend   Blend value, clamped to range [0, 1].
 */
template <typename SUB_CLASS, typename VALUE_CLASS>
detail::Arithmetic<SUB_CLASS, VALUE_CLASS> lerp(
    const detail::Arithmetic<SUB_CLASS, VALUE_CLASS>& from,
    const detail::Arithmetic<SUB_CLASS, VALUE_CLASS>& to,
    const typename detail::Arithmetic<SUB_CLASS, VALUE_CLASS>::value_t blend)
{
    return ((to - from) *= clamp(blend, 0, 1)) += from;
}

} // namespace notf
