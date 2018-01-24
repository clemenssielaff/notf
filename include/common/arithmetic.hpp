#pragma once

#include <array>
#include <cassert>

#include "common/half.hpp"
#include "common/hash.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief Empty base for all arithmetic types.
///
/// @paragraph Nomenclature.
///
/// A `component` is a single element of the arithmetic type.
/// In a vector it is a scalar, in a matrix, it is a vector.
///
/// An `element` on the other hand is always a scalar.
/// In a vector, the element is equivalent to a component, in a matrix, the element is a component of a component.
///
/// @paragraph Design.
///
///                        ArithmeticImpl (empty template)
///
///                          /                        \
///                    specializes                specializes
///                         |                          |
///
///               ArithmeticImpl (vector)        ArithmeticImpl (matrix)
///
///                              \                  /
///                             either           ...or
///                                \              /
///
///                            Arithmetic (common base)
///
///                                       |
///                                  specializes
///                                       |
///
///                               Value type<element_t>
///
///                                       |
///                                    typedef
///                                       |
///
///                               Concrete value type
///
template<typename self_t, typename element_t, typename component_t, size_t dim, typename = void>
struct ArithmeticImpl;

//====================================================================================================================//

/// @brief Base for all arithmetic types that contain only scalars (i.e. vectors).
template<typename self_t, typename element_t, typename component_t, size_t dim>
struct ArithmeticImpl<self_t, element_t, component_t, dim,
                      std::enable_if_t<std::is_same<element_t, component_t>::value>> {

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Value data array.
    std::array<component_t, dim> data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    ArithmeticImpl() = default;

    /// @brief Perfect forwarding constructor.
    template<typename... ARGS>
    ArithmeticImpl(ARGS&&... args) : data{std::forward<ARGS>(args)...}
    {}

    ArithmeticImpl(std::array<component_t, dim> data) : data(std::move(data)) {}

    /// @brief Create a vector with all elements set to the given value.
    /// @param value    Value to set.
    static self_t fill(const element_t value)
    {
        self_t result;
        result.data.fill(value);
        return result;
    }

    /// @brief Number of components in this vector.
    static constexpr size_t size() { return dim; }

    /// @brief Check whether this vector is of unit magnitude.
    bool is_unit() const { return abs(magnitude_sq() - 1) <= precision_high<element_t>(); }

    /// @brief Calculate the squared magnitude of this vector.
    element_t magnitude_sq() const
    {
        element_t result = 0;
        for (size_t i = 0; i < dim; ++i) {
            result += data[i] * data[i];
        }
        return result;
    }

    /// @brief Returns the magnitude of this vector.
    element_t magnitude() const { return sqrt(magnitude_sq()); }

    /// @brief Calculate a normalized copy of this vector.
    self_t normalize() const
    {
        const element_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<element_t>()) {
            self_t result; // is unit
            result.data = data;
            return result;
        }
        if (abs(mag_sq) <= precision_high<element_t>()) {
            return self_t::zero(); // is zero
        }
        return *reinterpret_cast<const self_t*>(this) / sqrt(mag_sq);
    }

    /// @brief Returns the dot product of this vector and another.
    /// @param other     Other vector.
    element_t dot(const self_t& other) const
    {
        const self_t tmp = *reinterpret_cast<const self_t*>(this) * other;
        element_t result = 0;
        for (size_t i = 0; i < dim; ++i) {
            result += tmp[i];
        }
        return result;
    }

    /// @brief Checks whether this vector is orthogonal to the other.
    /// @note The zero vector is orthogonal to every other vector.
    /// @param other     Vector to test against.
    bool is_orthogonal_to(const self_t& other) const
    {
        // normalization required for large absolute differences in vector lengths
        const self_t this_norm  = normalize();
        const self_t other_norm = other.normalize();
        return abs(this_norm.dot(other_norm)) <= precision_high<element_t>();
    }

    /// @brief Set all components of this vector to the given value.
    /// @param value    Value to set.
    self_t& set_all(const element_t value)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i] = value;
        }
        return *reinterpret_cast<self_t*>(this);
    }

    /// @brief Calulate the a component-wise maximum of this and the other vector.
    /// @param other    Other vector to max against.
    self_t max(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = std::max(data[i], other[i]);
        }
        return result;
    }

    /// @brief Calulate the a component-wise minimum of this and the other vector.
    /// @param other    Other vector to min against.
    self_t min(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = std::min(data[i], other[i]);
        }
        return result;
    }

    /// @brief Sum of all components of this vector.
    element_t sum() const
    {
        element_t result = 0;
        for (size_t i = 0; i < dim; ++i) {
            result += data[i];
        }
        return result;
    }

    /// @brief Tests whether all components of this vector are real values (not NAN, not INFINITY).
    bool is_real() const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!notf::is_real(data[i])) {
                return false;
            }
        }
        return true;
    }

    /// @brief Tests whether all components of this vector are close to or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    bool is_zero(const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /// @brief Tests whether any of the components of this vector is close to or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    bool contains_zero(const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i]) <= epsilon) {
                return true;
            }
        }
        return false;
    }

    /// @brief Tests whether this vector is component-wise close to the other.
    /// @param other    Other vector to test against.
    /// @param epsilon  Largest ignored difference.
    bool is_approx(const self_t& other, const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (abs(data[i] - other[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    /// @brief Raw pointer to the first address of the vector's data.
    element_t* as_ptr() { return &data[0]; }

    /// @brief Raw const pointer to the first address of the vector's data.
    const element_t* as_ptr() const { return &data[0]; }
};

//====================================================================================================================//

/// @brief Base for all arithmetic types that contain vectors (i.e. matrices).
template<typename self_t, typename element_t, typename component_t, size_t dim>
struct ArithmeticImpl<self_t, element_t, component_t, dim,
                      std::enable_if_t<!std::is_same<element_t, component_t>::value>> {

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Value data array.
    std::array<component_t, dim> data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    ArithmeticImpl() = default;

    /// @brief Perfect forwarding constructor.
    template<typename... ARGS>
    ArithmeticImpl(ARGS&&... args) : data{std::forward<ARGS>(args)...}
    {}

    /// @brief Create a matrix with all elements set to the given value.
    /// @param value    Value to set.
    static self_t fill(const element_t value)
    {
        self_t result;
        for (size_t i = 0; i < dim; ++i) {
            result[i].set_all(value);
        }
        return result;
    }

    /// @brief Number of elements in this matrix.
    static constexpr size_t size() { return dim * component_t::size(); }

    /// @brief Set all elements of this matrix to the given value.
    /// @param value    Value to set.
    self_t& set_all(const element_t value)
    {
        for (size_t i = 0; i < dim; ++i) {
            data[i].set_all(value);
        }
        return *reinterpret_cast<self_t*>(this);
    }

    /// @brief Calulate the a element-wise maximum of this and the other matrix.
    /// @param other    Other matrix to max against.
    self_t max(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = data[i].max(other[i]);
        }
        return result;
    }

    /// @brief Calulate the a element-wise minimum of this and the other matrix.
    /// @param other    Other matrix to min against.
    self_t min(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < dim; ++i) {
            result[i] = data[i].min(other[i]);
        }
        return result;
    }

    /// @brief Sum of all elements of this matrix.
    element_t sum() const
    {
        element_t result = 0;
        for (size_t i = 0; i < dim; ++i) {
            result += data[i].sum();
        }
        return result;
    }

    /// @brief Tests whether all elements of this matrix are real values (not NAN, not INFINITY).
    bool is_real() const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_real()) {
                return false;
            }
        }
        return true;
    }

    /// @brief Tests whether all elements of this matrix are close to, or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    bool is_zero(const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_zero(epsilon)) {
                return false;
            }
        }
        return true;
    }

    /// @brief Tests whether any of the elements of this matrix is close to, or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    bool contains_zero(const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (data[i].contains_zero(epsilon)) {
                return true;
            }
        }
        return false;
    }

    /// @brief Tests whether this matrix is element-wise close to the other.
    /// @param other    Other matrix to test against.
    /// @param epsilon  Largest ignored difference.
    bool is_approx(const self_t& other, const element_t epsilon = precision_high<element_t>()) const
    {
        for (size_t i = 0; i < dim; ++i) {
            if (!data[i].is_approx(other[i], epsilon)) {
                return false;
            }
        }
        return true;
    }

    /// @brief Raw pointer to the first address of the matrix's data.
    element_t* as_ptr() { return data[0].as_ptr(); }

    /// @brief Raw const pointer to the first address of the matrix's data.
    const element_t* as_ptr() const { return data[0].as_ptr(); }
};

//====================================================================================================================//

/// @brief Helper struct to extract the element type from a component type at compile time.
template<typename T, typename = void>
struct get_element_type {
    using type = typename T::element_t;
};
template<typename T>
struct get_element_type<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    using type = T;
};
template<>
struct get_element_type<half> {
    using type = half;
};

/// @brief Base for all arithmetic value types.
///
/// The Arithmetic base class provides naive implementations of each operation.
/// You can override specific functionality for value-specific behaviour.
template<typename SELF, typename COMPONENT, size_t DIMENSIONS>
struct Arithmetic : public ArithmeticImpl<SELF, typename get_element_type<COMPONENT>::type, COMPONENT, DIMENSIONS> {

    /// @brief Component type.
    using component_t = COMPONENT;

    /// @brief Element type.
    using element_t = typename get_element_type<component_t>::type;

    /// @brief Actual type constructed through this template.
    using self_t = SELF;

    /// @brief Base type, different for vectors and matrices.
    using super_t = ArithmeticImpl<self_t, element_t, component_t, DIMENSIONS>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Arithmetic() = default;

    /// @brief Perfect forwarding constructor.
    template<typename... T>
    Arithmetic(T&&... ts) : super_t{std::forward<T>(ts)...}
    {}

    /// @brief Set all elements to zero.
    static self_t zero() { return super_t::fill(0); }

    /// @brief Creates a copy of this instance.
    self_t copy() const { return *this; }

    /// @brief Calculates and returns the hash of this value.
    size_t hash() const
    {
        std::size_t result = 0;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            notf::hash_combine(result, data[i]);
        }
        return result;
    }

    /// @brief Read-write reference to a component of this value by index.
    /// @param index    Index of the requested component.
    component_t& operator[](const size_t index)
    {
        assert(index < DIMENSIONS);
        return data[index];
    }

    /// @brief Read-only reference to a component of this value by index.
    /// @param index    Index of the requested component.
    const component_t& operator[](const size_t index) const
    {
        assert(index < DIMENSIONS);
        return data[index];
    }

    /// @brief The contents of this value as a const array.
    const std::array<element_t, super_t::size()>& as_array() const
    {
        return *reinterpret_cast<const std::array<element_t, super_t::size()>*>(super_t::as_ptr());
    }

    /// @brief Set all elements to zero.
    self_t& set_zero() { return super_t::set_all(0); }

    /// @brief Equality operator.
    /// @param other    Value to test against.
    bool operator==(const self_t& other) const { return data == other.data; }

    /// @brief Inequality operator.
    /// @param other    Value to test against.
    bool operator!=(const self_t& other) const { return data != other.data; }

    /// @brief Sum of this value with other.
    /// @param other    Summand.
    self_t operator+(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }

    /// @brief In-place addition of other to this.
    /// @param other    Other summand.
    self_t& operator+=(const self_t& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] += other[i];
        }
        return _specialized_this();
    }

    /// @brief Difference between this value and other.
    /// @param other    Subtrahend.
    self_t operator-(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] - other[i];
        }
        return result;
    }

    /// @brief The in-place difference between this value and other.
    /// @param other    Subtrahend.
    self_t& operator-=(const self_t& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] -= other[i];
        }
        return _specialized_this();
    }

    /// @brief Component-wise multiplication of this value with other.
    /// @param other    Value to multiply with.
    self_t operator*(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] * other[i];
        }
        return result;
    }

    /// @brief In-place component-wise multiplication with another value.
    /// @param other    Value to multiply with.
    self_t& operator*=(const self_t& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] *= other[i];
        }
        return _specialized_this();
    }

    /// @brief Multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    self_t operator*(const element_t factor) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] * factor;
        }
        return result;
    }

    /// @brief In-place multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    self_t& operator*=(const element_t factor)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] *= factor;
        }
        return _specialized_this();
    }

    /// @brief Component-wise division of this value by other.
    /// @param other    Value to divide by.
    self_t operator/(const self_t& other) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] / other[i];
        }
        return result;
    }

    /// @brief In-place component-wise division of this value by other.
    /// @param other    Value to divide by.
    self_t& operator/=(const self_t& other)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] /= other[i];
        }
        return _specialized_this();
    }

    /// @brief Division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    self_t operator/(const element_t divisor) const
    {
        self_t result;
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            result[i] = data[i] / divisor;
        }
        return result;
    }

    /// @brief In-place division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    self_t& operator/=(const element_t divisor)
    {
        for (size_t i = 0; i < DIMENSIONS; ++i) {
            data[i] /= divisor;
        }
        return _specialized_this();
    }

    /// @brief The inverted value.
    self_t operator-() const { return *this * -1; }

protected:
    /// @brief Helper function converting this template into a concrete instance of the actual type.
    constexpr self_t& _specialized_this() { return *reinterpret_cast<self_t*>(this); }
};

} // namespace detail

//====================================================================================================================//

/// @brief Linear interpolation between two values.
///
/// @param from    Left value, full weight at blend <= 0.
/// @param to      Right value, full weight at blend >= 1.
/// @param blend   Blend value, clamped to range [0, 1].
template<typename value_t>
value_t lerp(const value_t& from, const value_t& to, const typename value_t::element_t blend)
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
