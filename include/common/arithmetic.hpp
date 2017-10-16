#pragma once

#include <array>
#include <assert.h>

#include "common/hash.hpp"

namespace notf {
namespace detail {

//*********************************************************************************************************************/

/** Helper struct to extract the data type from an element type at compile time. */
template <typename T, typename = void>
struct get_value_type {
    using type = typename T::value_t;
};
template <typename T>
struct get_value_type<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    using type = T;
};

//*********************************************************************************************************************/

/** Base class*/
template <typename T, typename value_t, typename element_t, size_t dim, typename = void>
struct ArithmeticImpl;

/** Base class for all arithmetic types that contain only scalars.
 * Some operations (like normalize) are only defined for vectors, not matrices.
 */
template <typename T, typename value_t, typename element_t, size_t dim>
struct ArithmeticImpl<T, value_t, element_t, dim, std::enable_if_t<std::is_same<value_t, element_t>::value>> {

    /** Value data array. */
    std::array<element_t, dim> data;

    ArithmeticImpl() = default;

    /** Perfect forwarding constructor. */
    template <typename... ARGS>
    ArithmeticImpl(ARGS&&... args)
        : data{std::forward<ARGS>(args)...} {}

public: // methods
    /** Returns an instance with all elements set to the given value. */
    static T fill(value_t value)
    {
        T result;
        result.data.fill(value);
        return result;
    }

    /** Checks whether this value is of unit magnitude. */
    bool is_unit() const { return abs(magnitude_sq() - 1) <= precision_high<value_t>(); }

    /** Returns the squared magnitude of this value. */
    value_t magnitude_sq() const
    {
        value_t result = 0;
        for (size_t i = 0; i < dim; ++i) {
            result += data[i] * data[i];
        }
        return result;
    }

    /** Returns the magnitude of this value. */
    value_t magnitude() const { return sqrt(magnitude_sq()); }

    /** A normalized copy of this value. */
    T normalize() const
    {
        const value_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<value_t>()) {
            T result;
            result.data = data;
            return result; // is unit
        }
        if (abs(mag_sq) <= precision_high<value_t>()) {
            return T::zero(); // is zero
        }
        return *reinterpret_cast<const T*>(this) / sqrt(mag_sq);
    }

protected: // methods
    static constexpr size_t size() { return dim; }

    void set_all(const value_t value)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i] = value;
        }
    }

    T& max(const T& other)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i] = notf::max(data[i], other[i]);
        }
        return *reinterpret_cast<T*>(this);
    }

    T max(const T& other) const
    {
        T result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = notf::max(data[i], other[i]);
        }
        return result;
    }

    T& min(const T& other)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i] = notf::min(data[i], other[i]);
        }
        return *reinterpret_cast<T*>(this);
    }

    T min(const T& other) const
    {
        T result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = notf::min(data[i], other[i]);
        }
        return result;
    }

    bool is_real() const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!notf::is_real(data[i])) {
                return false;
            }
        }
        return true;
    }

    bool is_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    bool contains_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i]) <= epsilon) {
                return true;
            }
        }
        return false;
    }

    bool is_approx(const T& other, const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i] - other[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    const value_t* as_ptr() const { return &data[0]; }

    value_t* as_ptr() { return &data[0]; }
};

/** Vectors containing other vectors. */
template <typename T, typename value_t, typename element_t, size_t dim>
struct ArithmeticImpl<T, value_t, element_t, dim, std::enable_if_t<!std::is_same<value_t, element_t>::value>> {

    /** Value data array. */
    std::array<element_t, dim> data;

    ArithmeticImpl() = default;

    /** Perfect forwarding constructor. */
    template <typename... ARGS>
    ArithmeticImpl(ARGS&&... args)
        : data{std::forward<ARGS>(args)...} {}

public: // methods
    /** Returns an instance with all elements set to the given value. */
    static T fill(value_t value)
    {
        T result;
        using Element = element_t;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = Element::fill(value);
        }
        return result;
    }

protected: // methods
    static constexpr size_t size() { return dim * element_t::size(); }

    void set_all(const value_t value)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i].set_all(value);
        }
    }

    T& max(const T& other)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i].max(other[i]);
        }
        return *reinterpret_cast<T*>(this);
    }

    T max(const T& other) const
    {
        T result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = data[i].max(other[i]);
        }
        return result;
    }

    T& min(const T& other)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i].min(other[i]);
        }
        return *reinterpret_cast<T*>(this);
    }

    T min(const T& other) const
    {
        T result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = data[i].min(other[i]);
        }
        return result;
    }

    bool is_real() const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_real()) {
                return false;
            }
        }
        return true;
    }

    bool is_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_zero(epsilon)) {
                return false;
            }
        }
        return true;
    }

    bool contains_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (data[i].contains_zero(epsilon)) {
                return true;
            }
        }
        return false;
    }

    bool is_approx(const T& other, const value_t epsilon = precision_high<value_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_approx(other[i], epsilon)) {
                return false;
            }
        }
        return true;
    }

    const value_t* as_ptr() const { return data[0].as_ptr(); }

    value_t* as_ptr() { return data[0].as_ptr(); }
};

//*********************************************************************************************************************/

/** Base class for all arithmetic value types.
 *
 * The Arithmetic base class provides naive implementations of each operation.
 * You can override specific functionality for value-specific behaviour or to make use of SIMD instructions.
 */
template <typename SPECIALIZATION, typename ELEMENT, size_t DIMENSIONS, bool SIMD_SPECIALIZATION = false>
struct Arithmetic : public ArithmeticImpl<SPECIALIZATION, typename get_value_type<ELEMENT>::type, ELEMENT, DIMENSIONS> {

    /* Types **********************************************************************************************************/

    using implementation = ArithmeticImpl<SPECIALIZATION, typename get_value_type<ELEMENT>::type, ELEMENT, DIMENSIONS>;

    /** Element type.
     * In a vector, this is equal to value_t, in a matrix this is the vector type.
     */
    using element_t = ELEMENT;

    /** Data type. */
    using value_t = typename get_value_type<ELEMENT>::type;

    using implementation::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    Arithmetic() = default;

    /** Perfect forwarding constructor. */
    template <typename... T>
    Arithmetic(T&&... ts)
        : implementation{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Set all elements to zero. */
    static SPECIALIZATION zero() { return implementation::fill(0); }

    /* Inspection  ****************************************************************************************************/

    /** Dimensions of the value. */
    static constexpr size_t size() { return implementation::size(); }

    /** Creates a copy of this instance. */
    SPECIALIZATION copy() const { return *this; }

    /** Checks, if this value contains only real, finite values (no INFINITY or NAN). */
    bool is_real() const { return implementation::is_real(); }

    /** Returns true if all elements are (approximately) zero.
     * @param epsilon   Largest difference that is still considered to be zero.
     */
    bool is_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        return implementation::is_zero(epsilon);
    }

    /** Checks, if any element of this value is (approximately) zero. */
    bool contains_zero(const value_t epsilon = precision_high<value_t>()) const
    {
        return implementation::contains_zero(epsilon);
    }

    /** Component-wise check if this value is approximately the same as another.
     * @param other     Value to test against.
     * @param epsilon   Maximal allowed distance between the two values.
     */
    bool is_approx(const SPECIALIZATION& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return implementation::is_approx(other, epsilon);
    }

    /** Calculates and returns the hash of this value. */
    size_t hash() const
    {
        std::size_t result = 0;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            notf::hash_combine(result, data[i]);
        }
        return result;
    }

    /** Checks whether two values are equal. */
    bool operator==(const SPECIALIZATION& other) const { return data == other.data; }

    /** Checks whether two values are not equal. */
    bool operator!=(const SPECIALIZATION& other) const { return data != other.data; }

    /** Read-only reference to an element of this value by index. */
    const element_t& operator[](const size_t index) const
    {
        assert(index < DIMENSIONS);
        return data[index];
    }

    /** Read-only pointer to the value's internal storage. */
    const value_t* as_ptr() const { return implementation::as_ptr(); }

    /** The contents of this value as an array. */
    const std::array<value_t, size()>& as_array() const
    {
        return *reinterpret_cast<const std::array<value_t, size()>*>(as_ptr());
    }

    /** Modification **************************************************************************************************/

    /** Read-write reference to an element of this value by index. */
    element_t& operator[](const size_t index)
    {
        assert(index < DIMENSIONS);
        return data[index];
    }

    /** Read-write pointer to the value's internal storage. */
    value_t* as_ptr() { return implementation::as_ptr(); }

    /** Set all elements of this value. */
    SPECIALIZATION& set_all(const value_t value)
    {
        implementation::set_all(value);
        return _specialized_this();
    }

    /** Set all elements to zero. */
    SPECIALIZATION& set_zero() { return set_all(0); }

    /** The sum of this value with another one. */
    SPECIALIZATION operator+(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] += other[i];
        }
        return result;
    }

    /** In-place addition of another value. */
    SPECIALIZATION& operator+=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] += other[i];
        }
        return _specialized_this();
    }

    /** The difference between this value and another one. */
    SPECIALIZATION operator-(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] -= other[i];
        }
        return result;
    }

    /** In-place subtraction of another value. */
    SPECIALIZATION& operator-=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] -= other[i];
        }
        return _specialized_this();
    }

    /** Component-wise multiplication of this value with another. */
    SPECIALIZATION operator*(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] *= other[i];
        }
        return result;
    }

    /** In-place component-wise multiplication with another value. */
    SPECIALIZATION& operator*=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] *= other[i];
        }
        return _specialized_this();
    }

    /** Multiplication of this value with a scalar. */
    SPECIALIZATION operator*(const value_t factor) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] *= factor;
        }
        return result;
    }

    /** In-place multiplication with a scalar. */
    SPECIALIZATION& operator*=(const value_t factor)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] *= factor;
        }
        return _specialized_this();
    }

    /** Component-wise division of this value by another. */
    SPECIALIZATION operator/(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] /= other[i];
        }
        return result;
    }

    /** In-place component-wise division by another value. */
    SPECIALIZATION& operator/=(const SPECIALIZATION& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] /= other[i];
        }
        return _specialized_this();
    }

    /** Division of this value by a scalar. */
    SPECIALIZATION operator/(const value_t divisor) const
    {
        SPECIALIZATION result;
        result.data = data;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] /= divisor;
        }
        return result;
    }

    /** In-place division by a scalar. */
    SPECIALIZATION& operator/=(const value_t divisor)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] /= divisor;
        }
        return _specialized_this();
    }

    /** The inverted value. */
    SPECIALIZATION operator-() const { return *this * -1; }

    SPECIALIZATION max(const SPECIALIZATION& other) const
    {
        return implementation::max(other);
    }

    SPECIALIZATION min(const SPECIALIZATION& other) const
    {
        return implementation::min(other);
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
template <typename T>
T lerp(const T& from, const T& to, const typename T::value_t blend)
{
    if (blend <= 0) {
        return from;
    }
    else if (blend >= 1) {
        return to;
    }
    return ((to - from) *= blend) += from;
}

} // namespace notf
