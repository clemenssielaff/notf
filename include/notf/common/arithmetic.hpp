#pragma once

#include <array>

#include "notf/meta/assert.hpp"
#include "notf/meta/hash.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

// arithmetic identifier ============================================================================================ //

namespace detail {

struct ArithmeticIdentifier { // TODO: use type detectors from concepts.hpp

    /// Tests if a given type is an arithmetic type.
    template<class T>
    static constexpr auto test() {
        if constexpr (std::disjunction_v<std::negation<decltype(has_derived_t<T>(declval<T>()))>,
                                         std::negation<decltype(has_element_t<T>(declval<T>()))>,
                                         std::negation<decltype(has_component_t<T>(declval<T>()))>,
                                         std::negation<decltype(has_get_dimensions<T>(declval<T>()))>>) {
            return std::false_type{}; // not an arithmetic type
        } else {
            using derived_t = typename T::derived_t;
            using element_t = typename T::element_t;
            using component_t = typename T::component_t;
            const size_t dim = T::get_dimensions();
            return std::is_convertible<T*, Arithmetic<derived_t, element_t, component_t, dim>*>{};
        }
    }

    /// Checks if a given arithmetic type can be converted into another.
    template<class From, class To>
    static constexpr bool is_convertible() {
        if constexpr (!test<From>() || !test<To>()) {
            return false; // not an arithmetic type
        } else if constexpr (From::get_dimensions() != To::get_dimensions()) {
            return false; // different dimensions
        } else if constexpr (!std::is_convertible_v<typename From::component_t, typename To::component_t>) {
            return false; // incompatible data types
        } else {
            return true;
        }
    }

    /// Checks, whether the given type has a nested type `derived_t`.
    template<class T>
    static constexpr auto has_derived_t(const T&) -> decltype(declval<typename T::derived_t>(), std::true_type{});
    template<class>
    static constexpr auto has_derived_t(...) -> std::false_type;

    /// Checks, whether the given type has a nested type `element_t`.
    template<class T>
    static constexpr auto has_element_t(const T&) -> decltype(declval<typename T::element_t>(), std::true_type{});
    template<class>
    static constexpr auto has_element_t(...) -> std::false_type;

    /// Checks, whether the given type has a nested type `component_t`.
    template<class T>
    static constexpr auto has_component_t(const T&) -> decltype(declval<typename T::component_t>(), std::true_type{});
    template<class>
    static constexpr auto has_component_t(...) -> std::false_type;

    /// Checks, whether the given type has a static method `get_dimensions`.
    template<class T>
    static constexpr auto has_get_dimensions(const T&) -> decltype(T::get_dimensions(), std::true_type{});
    template<class>
    static constexpr auto has_get_dimensions(...) -> std::false_type;
};

template<class T>
static constexpr bool is_arithmetic_type = decltype(ArithmeticIdentifier::test<T>())::value;

// arithmetic base ================================================================================================== //

template<class Derived, class Element, class Component, size_t Dimensions>
struct Arithmetic {

    static_assert(Dimensions > 0, "Cannot define a zero-dimensional arithmetic type");

    // helper ---------------------------------------------------------------------------------- //
private:
    static constexpr bool _is_ground() { return std::is_same_v<element_t, component_t>; }

    // types ----------------------------------------------------------------------------------- //
public:
    /// Arithmetic specialization.
    using derived_t = Derived;

    /// Scalar type used by this arithmetic type.
    using element_t = Element;

    /// Component type used by this arithmetic type.
    /// In a vector, this is the same as element_t, whereas in a matrix it will be a vector.
    using component_t = Component;

    /// Data holder.
    using Data = std::array<component_t, Dimensions>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Arithmetic() noexcept = default;

    /// Value constructor.
    /// @param data Raw data for this arithmetic type.
    constexpr Arithmetic(Data data) noexcept : data(std::move(data)) {}

    /// Copy constructor for any compatible arithmetic type.
    template<class T, class = std::enable_if_t<ArithmeticIdentifier::is_convertible<derived_t, T>()>>
    constexpr Arithmetic(const T& other) noexcept {
        for (size_t i = 0; i < get_size(); ++i) {
            data[i] = other.data[i];
        }
    }

    /// Value constructor.
    /// @param args     Up to (Dimension) arguments, each of which must be trivially convertible to component_t.
    ///                 Remaining components are value-initialized.
    template<class... Args, class = std::enable_if_t<(sizeof...(Args) <= Dimensions)
                                                     && (std::is_trivially_constructible_v<component_t, Args> && ...)>>
    constexpr Arithmetic(Args&&... args) noexcept : data{static_cast<component_t>(std::forward<Args>(args))...} {}

    /// Create an arithmetic value with all elements set to the given value.
    /// @param value    Value to set.
    constexpr static derived_t all(const element_t value) {
        derived_t result{};
        if constexpr (_is_ground()) {
            result.set_all(value);
        } else {
            for (size_t i = 0; i < get_dimensions(); ++i) {
                result[i].set_all(value);
            }
        }
        return result;
    }

    /// Arithmetic value with all elements set to zero.
    constexpr static derived_t zero() { return all(0); }

    // inspection -------------------------------------------------------------

    /// Number of components in this arithmetic type.
    static constexpr size_t get_dimensions() noexcept { return Dimensions; }

    /// Number of elements in this arithmetic type.
    static constexpr size_t get_size() noexcept {
        if constexpr (_is_ground()) {
            return get_dimensions();
        } else {
            return get_dimensions() * component_t::get_dimensions();
        }
    }

    /// @{
    /// Access to a component of this value by index.
    /// @param index    Index of the requested component.
    constexpr component_t& operator[](const size_t index) {
        NOTF_ASSERT(index < get_dimensions());
        return data[index];
    }
    constexpr const component_t& operator[](const size_t index) const {
        NOTF_ASSERT(index < get_dimensions());
        return data[index];
    }
    /// @}

    /// @{
    /// Raw pointer to the first address of the value's data.
    const element_t* as_ptr() const {
        if constexpr (_is_ground()) {
            return &data[0];
        } else {
            return data[0].as_ptr();
        }
    }
    element_t* as_ptr() {
        if constexpr (_is_ground()) {
            return &data[0];
        } else {
            return data[0].as_ptr();
        }
    }
    /// @}

    /// Tests whether all components of this value are real (not NAN, not INFINITY).
    constexpr bool is_real() const noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                if (!notf::is_real(data[i])) { return false; }
            } else {
                if (!data[i].is_real()) { return false; }
            }
        }
        return true;
    }

    /// Tests whether all components of this value are close to or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    constexpr bool is_zero(const element_t epsilon = precision_high<element_t>()) const noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                if (abs(data[i]) > epsilon) { return false; }
            } else {
                if (!data[i].is_zero(epsilon)) { return false; }
            }
        }
        return true;
    }

    /// Tests whether any of the components of this value is close to or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    constexpr bool contains_zero(const element_t epsilon = precision_high<element_t>()) const noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                if (abs(data[i]) <= epsilon) { return true; }
            } else {
                if (data[i].contains_zero(epsilon)) { return true; }
            }
        }
        return false;
    }

    /// Calculates and returns the hash of this value.
    constexpr size_t get_hash() const noexcept {
        std::size_t result = versioned_base_hash();
        for (size_t i = 0; i < get_dimensions(); ++i) {
            notf::hash_combine(result, data[i]);
        }
        return result;
    }

    // comparison -------------------------------------------------------------

    /// Tests whether this value is element-wise approximately equal to another.
    /// @param other    Other matrix to test against.
    /// @param epsilon  Largest ignored difference.
    constexpr bool is_approx(const derived_t& other, const element_t epsilon = precision_high<element_t>()) const
        noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                if (!notf::is_approx(data[i], other[i], epsilon)) { return false; }
            } else {
                if (!data[i].is_approx(other[i], epsilon)) { return false; }
            }
        }
        return true;
    }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const derived_t& other) const noexcept { return data == other.data; }

    /// Inequality operator.
    /// @param other    Value to test against.
    constexpr bool operator!=(const derived_t& other) const noexcept { return data != other.data; }

    // element/component-wise -------------------------------------------------

    /// Get the element-wise maximum of this and the other value.
    /// @param other    Other value to max against.
    constexpr derived_t& max(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                data[i] = std::max(data[i], other[i]);
            } else {
                data[i].max(other[i]);
            }
        }
        return (*static_cast<derived_t*>(this));
    }

    /// @{
    /// Get the element-wise maximum of this and the other value in a new value.
    /// @param other    Other value to max against.
    constexpr derived_t get_max(const derived_t& other) const& noexcept {
        derived_t result = *this;
        result.max(other);
        return result;
    }
    constexpr derived_t get_max(const derived_t& other) && noexcept { return this->max(other); }
    /// @}

    /// Get the element-wise minimum of this and the other value.
    /// @param other    Other value to min against.
    constexpr derived_t& min(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                data[i] = std::min(data[i], other[i]);
            } else {
                data[i].min(other[i]);
            }
        }
        return (*static_cast<derived_t*>(this));
    }

    /// @{
    /// Get the element-wise minimum of this and the other value in a new value.
    /// @param other    Other value to min against.
    constexpr derived_t get_min(const derived_t& other) const& noexcept {
        derived_t result = *this;
        result.min(other);
        return result;
    }
    constexpr derived_t get_min(const derived_t& other) && noexcept { return this->min(other); }
    /// @}

    /// Sum of all elements of this value.
    constexpr element_t get_sum() const noexcept {
        element_t result = 0;
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                result += data[i];
            } else {
                result += data[i].get_sum();
            }
        }
        return result;
    }

    /// Set all elements of this value to the given element.
    /// @param value    Value to set.
    constexpr derived_t& set_all(const element_t value) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (_is_ground()) {
                data[i] = value;
            } else {
                data[i].set_all(value);
            }
        }
        return (*static_cast<derived_t*>(this));
    }

    /// Set all components of this value to the given component.
    /// @param value    Value to set.
    template<bool IsNotGround = !_is_ground()>
    constexpr std::enable_if_t<IsNotGround, derived_t&> set_all(const component_t value) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] = value;
        }
        return (*static_cast<derived_t*>(this));
    }

    // element arithmetic -----------------------------------------------------

    /// In-place multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    constexpr derived_t& operator*=(const element_t factor) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] *= factor;
        }
        return *static_cast<derived_t*>(this);
    }

    /// In-place division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    constexpr derived_t& operator/=(const element_t divisor) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] /= divisor;
        }
        return *static_cast<derived_t*>(this);
    }

    /// @{
    /// Multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    constexpr derived_t operator*(const element_t factor) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] * factor;
        }
        return result;
    }
    constexpr derived_t operator*(const element_t factor) && noexcept { return *this *= factor; }
    /// @}

    /// @{
    /// Division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    constexpr derived_t operator/(const element_t divisor) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] / divisor;
        }
        return result;
    }
    constexpr derived_t operator/(const element_t divisor) && noexcept { return *this /= divisor; }
    /// @}

    /// The inverted value.
    constexpr derived_t operator-() const noexcept { return *this * -1; }

    // value arithmetic -------------------------------------------------------

    /// In-place addition of other to this.
    /// @param other    Other summand.
    constexpr derived_t& operator+=(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] += other[i];
        }
        return *static_cast<derived_t*>(this);
    }

    /// The in-place difference between this value and other.
    /// @param other    Subtrahend.
    constexpr derived_t& operator-=(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] -= other[i];
        }
        return *static_cast<derived_t*>(this);
    }

    /// In-place component-wise multiplication with another value.
    /// @param other    Value to multiply with.
    constexpr derived_t& operator*=(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] *= other[i];
        }
        return *static_cast<derived_t*>(this);
    }

    /// In-place component-wise division of this value by other.
    /// @param other    Value to divide by.
    constexpr derived_t& operator/=(const derived_t& other) noexcept {
        for (size_t i = 0; i < get_dimensions(); ++i) {
            data[i] /= other[i];
        }
        return *static_cast<derived_t*>(this);
    }

    /// @{
    /// Sum of this value with other.
    /// @param other    Summand.
    constexpr derived_t operator+(const derived_t& other) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }
    constexpr derived_t operator+(const derived_t& other) && noexcept { return *this += other; }
    /// @}

    /// @{
    /// Difference between this value and other.
    /// @param other    Subtrahend.
    constexpr derived_t operator-(const derived_t& other) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] - other[i];
        }
        return result;
    }
    constexpr derived_t operator-(const derived_t& other) && noexcept { return *this -= other; }
    /// @}

    /// @{
    /// Component-wise multiplication of this value with other.
    /// @param other    Value to multiply with.
    constexpr derived_t operator*(const derived_t& other) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] * other[i];
        }
        return result;
    }
    constexpr derived_t operator*(const derived_t& other) && noexcept { return *this *= other; }
    /// @}

    /// @{
    /// Component-wise division of this value by other.
    /// @param other    Value to divide by.
    constexpr derived_t operator/(const derived_t& other) const& noexcept {
        derived_t result{};
        for (size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] / other[i];
        }
        return result;
    }
    constexpr derived_t operator/(const derived_t& other) && noexcept { return *this /= other; }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    Data data;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
