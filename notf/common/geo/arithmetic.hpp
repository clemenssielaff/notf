#pragma once

#include "notf/meta/array.hpp"
#include "notf/meta/concept.hpp"
#include "notf/meta/hash.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// arithmetic  ====================================================================================================== //

template<class Actual, class Component, std::size_t Dimensions>
struct Arithmetic {

    static_assert(Dimensions > 0, "Cannot define a zero-dimensional arithmetic type");

    // helper ---------------------------------------------------------------------------------- //
private:
    struct _detail {
        template<class T, class = void>
        struct produce_element {
            using type = T;
        };
        template<class T>
        struct produce_element<T, std::void_t<typename T::element_t>> {
            using type = typename T::element_t;
        };
    };

    // types ----------------------------------------------------------------------------------- //
public:
    /// Arithmetic specialization deriving from this base class.
    using actual_t = Actual;

    /// Component type used by this arithmetic type.
    /// In a vector, this is the same as element_t, whereas in a matrix it will be a vector.
    using component_t = Component;

    /// Scalar type used by this arithmetic type.
    using element_t = typename _detail::template produce_element<component_t>::type;

    /// Number of components stored in this arithmetic type.
    constexpr static const std::size_t dimensions = Dimensions;

    /// Data holder.
    using Data = std::array<component_t, Dimensions>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    /// Usually leaves the data in an undefined state.
    constexpr Arithmetic() noexcept = default;

    /// Value constructor.
    /// @param data Raw data for this arithmetic type.
    constexpr Arithmetic(Data data) noexcept : data(std::move(data)) {}

    /// Component constructor.
    /// Allows construction of a Vector3<float> using (0, 1.3f, 123.l), for example.
    /// @param components   Components to store.
    template<class... Components, class = std::enable_if_t<all(sizeof...(Components) == Dimensions, //
                                                               all_convertible_to_v<component_t, Components...>)>>
    constexpr Arithmetic(Components... components) noexcept : data{static_cast<component_t>(components)...} {}

    /// Casting constructor.
    /// Allows implicit casting from Size2<int> to Size2<float>, for example.
    /// @param similar  Similar data type using a different element type.
    template<class Similar, class T,
             class = std::enable_if_t<all(!std::is_same_v<Actual, Similar>,
                                          is_static_castable_v<typename Similar::element_t, element_t>)>>
    constexpr Arithmetic(const Arithmetic<Similar, T, Dimensions>& similar) noexcept {
        for (size_t i = 0; i < Dimensions; ++i) {
            data[i] = static_cast<element_t>(similar.data[i]);
        }
    }

    /// Create an arithmetic value with all elements set to the given value.
    /// @param value    Value to set.
    constexpr static actual_t all(const element_t value) noexcept {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return {make_array_of<get_dimensions()>(value)};
        } else {
            return {make_array_of<get_dimensions()>(component_t::all(value))};
        }
    }

    /// Create an arithmetic value with all elements set to the highest possible value.
    constexpr static actual_t highest() noexcept { return all(highest_v<element_t>); }

    /// Create an arithmetic value with all elements set to the lowest possible value.
    constexpr static actual_t lowest() noexcept { return all(lowest_v<element_t>); }

    /// Arithmetic value with all elements set to zero.
    constexpr static actual_t zero() noexcept { return {Data{}}; }

    // inspection -------------------------------------------------------------

    /// Number of components in this arithmetic type.
    static constexpr std::size_t get_dimensions() noexcept { return Dimensions; }

    /// Number of elements in this arithmetic type.
    static constexpr std::size_t get_size() noexcept {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return get_dimensions();
        } else {
            return get_dimensions() * component_t::get_size();
        }
    }

    /// Calculates and returns the hash of this value.
    constexpr std::size_t get_hash() const noexcept {
        std::size_t result = detail::versioned_base_hash();
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                notf::hash_combine(result, data[i]);
            } else {
                notf::hash_combine(result, data[i].get_hash());
            }
        }
        return result;
    }

    /// @{
    /// Access to a component of this value by index.
    /// @param index    Index of the requested component.
    constexpr component_t& operator[](const std::size_t index) noexcept(!config::is_debug_build()
                                                                   || config::abort_on_assert()) {
        NOTF_ASSERT(index < get_dimensions());
        return data[index];
    }
    constexpr const component_t& operator[](const std::size_t index) const
        noexcept(!config::is_debug_build() || config::abort_on_assert()) {
        NOTF_ASSERT(index < get_dimensions());
        return data[index];
    }
    /// @}

    /// @{
    /// Raw pointer to the first address of the value's data.
    constexpr const element_t* as_ptr() const noexcept {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return &data[0];
        } else {
            return data[0].as_ptr();
        }
    }
    constexpr element_t* as_ptr() noexcept {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return &data[0];
        } else {
            return data[0].as_ptr();
        }
    }
    /// @}

    /// Tests whether all components of this value are close to or equal to, zero.
    /// @param epsilon  Largest ignored difference.
    constexpr bool is_zero(const element_t epsilon = precision_high<element_t>()) const noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
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
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                if (abs(data[i]) <= epsilon) { return true; }
            } else {
                if (data[i].contains_zero(epsilon)) { return true; }
            }
        }
        return false;
    }

    /// Tests whether all components of this value are real (not NAN, not INFINITY).
    constexpr bool is_real() const noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                if (!notf::is_real(data[i])) { return false; }
            } else {
                if (!data[i].is_real()) { return false; }
            }
        }
        return true;
    }

    // comparison -------------------------------------------------------------

    /// Tests whether this value is element-wise approximately equal to another.
    /// @param other    Other matrix to test against.
    /// @param epsilon  Largest ignored difference.
    constexpr bool is_approx(const actual_t& other,
                             const element_t epsilon = precision_high<element_t>()) const noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                if (!notf::is_approx(data[i], other[i], epsilon)) { return false; }
            } else {
                if (!data[i].is_approx(other[i], epsilon)) { return false; }
            }
        }
        return true;
    }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const actual_t& other) const noexcept { return data == other.data; }

    /// Inequality operator.
    /// @param other    Value to test against.
    constexpr bool operator!=(const actual_t& other) const noexcept { return data != other.data; }

    // element/component-wise -------------------------------------------------

    /// Get the element-wise maximum of this and other.
    /// @param other    Other value to max against.
    constexpr actual_t get_max(const actual_t& other) const noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                result.data[i] = notf::max(data[i], other[i]);
            } else {
                result.data[i] = data[i].get_max(other[i]);
            }
        }
        return result;
    }

    /// Get the element-wise minimum of this and other.
    /// @param other    Other value to min against.
    constexpr actual_t get_min(const actual_t& other) const noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                result.data[i] = notf::min(data[i], other[i]);
            } else {
                result.data[i] = data[i].get_min(other[i]);
            }
        }
        return result;
    }

    /// Sum of all elements of this value.
    constexpr element_t get_sum() const noexcept {
        element_t result = 0;
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                result += data[i];
            } else {
                result += data[i].get_sum();
            }
        }
        return result;
    }

    /// Set all elements of this value to the given element.
    /// @param value    Value to set.
    constexpr actual_t& set_all(const element_t value) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                data[i] = value;
            } else {
                data[i].set_all(value);
            }
        }
        return *static_cast<actual_t*>(this);
    }

    /// Set all elements of this value to the element-wise maximum of this and other.
    /// @param other    Other value to max against.
    constexpr actual_t& maximize_with(const actual_t& other) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                data[i] = notf::max(data[i], other[i]);
            } else {
                data[i].maximize_with(other[i]);
            }
        }
        return *static_cast<actual_t*>(this);
    }

    /// Set all elements of this value to the element-wise minimum of this and other.
    /// @param other    Other value to min against.
    constexpr actual_t& minimize_with(const actual_t& other) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                data[i] = notf::min(data[i], other[i]);
            } else {
                data[i].minimize_with(other[i]);
            }
        }
        return *static_cast<actual_t*>(this);
    }

    /// Set all elements of this value to their absolute value.
    constexpr actual_t& set_absolute() noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                data[i] = notf::abs(data[i]);
            } else {
                data[i].set_absolute();
            }
        }
        return *static_cast<actual_t*>(this);
    }

    /// Get a copy of this value with all elements set to their absolute value.
    constexpr actual_t get_absolute() const & noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            if constexpr (std::is_arithmetic_v<component_t>) {
                result.data[i] = notf::abs(data[i]);
            } else {
                result.data[i] = data[i].get_absolute();
            }
        }
        return result;
    }
    constexpr actual_t& get_absolute() && noexcept {
        return set_absolute();
    }

    // scalar arithmetic ------------------------------------------------------

    /// @{
    /// Multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    constexpr actual_t operator*(const element_t factor) const& noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] * factor;
        }
        return result;
    }
    constexpr actual_t operator*(const element_t factor) && noexcept { return *this *= factor; }
    friend constexpr actual_t operator*(const element_t factor, const Arithmetic& rhs) noexcept { return rhs * factor; }
    friend constexpr actual_t operator*(const element_t factor, Arithmetic&& rhs) noexcept { return rhs * factor; }
    /// @}

    /// In-place multiplication of this value with a scalar.
    /// @param factor   Factor to multiply with.
    constexpr actual_t& operator*=(const element_t factor) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            data[i] *= factor;
        }
        return *static_cast<actual_t*>(this);
    }

    /// @{
    /// Division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    constexpr actual_t operator/(const element_t divisor) const& noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] / divisor;
        }
        return result;
    }
    constexpr actual_t operator/(const element_t divisor) && noexcept { return *this /= divisor; }
    /// @}

    /// In-place division of this value by a scalar.
    /// @param divisor  Divisor to divide by.
    constexpr actual_t& operator/=(const element_t divisor) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            data[i] /= divisor;
        }
        return *static_cast<actual_t*>(this);
    }

    /// @{
    /// The inverted value.
    constexpr actual_t operator-() const& noexcept { return *this * -1; }
    constexpr actual_t operator-() const&& noexcept { return *this *= -1; }
    /// @}

    // value arithmetic -------------------------------------------------------

    /// @{
    /// Sum of this value with other.
    /// @param other    Summand.
    constexpr actual_t operator+(const actual_t& other) const& noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }
    constexpr actual_t operator+(const actual_t& other) && noexcept { return *this += other; }
    /// @}

    /// In-place addition of other to this.
    /// @param other    Other summand.
    constexpr actual_t& operator+=(const actual_t& other) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            data[i] += other[i];
        }
        return *static_cast<actual_t*>(this);
    }

    /// @{
    /// Difference between this value and other.
    /// @param other    Subtrahend.
    constexpr actual_t operator-(const actual_t& other) const& noexcept {
        actual_t result{};
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            result[i] = data[i] - other[i];
        }
        return result;
    }
    constexpr actual_t operator-(const actual_t& other) && noexcept { return *this -= other; }
    /// @}

    /// The in-place difference between this value and other.
    /// @param other    Subtrahend.
    constexpr actual_t& operator-=(const actual_t& other) noexcept {
        for (std::size_t i = 0; i < get_dimensions(); ++i) {
            data[i] -= other[i];
        }
        return *static_cast<actual_t*>(this);
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    Data data;
};

// arithmetic vector ================================================================================================ //

/// Arithmetic vector implementation.
template<class Actual, class Component, std::size_t Dimensions>
struct ArithmeticVector : public Arithmetic<Actual, Component, Dimensions> {

    // types --------------------------------------------------------------------------------- //
private:
    /// Base class.
    using super_t = Arithmetic<Actual, Component, Dimensions>;

public:
    /// Arithmetic specialization deriving from this base class.
    using actual_t = Actual;

    /// Component type used by this arithmetic type.
    /// In a vector, this is the same as element_t, whereas in a matrix it will be a vector.
    using component_t = typename super_t::component_t;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Data holder.
    using Data = typename super_t::Data;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr ArithmeticVector() noexcept = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr ArithmeticVector(Args&&... args) noexcept : super_t(std::forward<Args>(args)...) {}

    // magnitude --------------------------------------------------------------

    /// Check whether this vector is of unit magnitude.
    constexpr bool is_unit() const noexcept { return abs(get_magnitude_sq() - 1) <= precision_high<element_t>(); }

    /// Calculate the squared magnitude of this vector.
    constexpr element_t get_magnitude_sq() const noexcept { return this->dot(*this); }

    /// Returns the magnitude of this vector.
    constexpr element_t get_magnitude() const noexcept { return sqrt(get_magnitude_sq()); }

    /// Normalizes this vector in-place.
    actual_t& normalize() noexcept {
        const element_t mag_sq = get_magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<element_t>()) { // is unit
            return *static_cast<actual_t*>(this);
        }
        if (abs(mag_sq) <= precision_high<element_t>()) { // is zero
            return *static_cast<actual_t*>(this);
        }
        return *this /= sqrt(mag_sq);
    }

    /// @{
    /// Returns a normalized copy of this vector.
    constexpr actual_t get_normalized() const& noexcept {
        actual_t result(data);
        result.normalize();
        return result;
    }
    actual_t& get_normalized() && noexcept { return normalize(); }
    /// @}

    /// Normalizes this vector in-place.
    /// Is fast but imprecise.
    actual_t& fast_normalize() noexcept { return *this /= notf::fast_inv_sqrt(get_magnitude_sq()); }

    /// @{
    /// Returns an imprecisely normalized copy of this vector.
    constexpr actual_t get_fast_normalized() const& noexcept {
        actual_t result(data);
        result.fast_normalize();
        return result;
    }
    actual_t& get_fast_normalized() && noexcept { return fast_normalize(); }
    /// @}

    // geometric --------------------------------------------------------------

    /// Returns the dot product of this vector and another.
    /// @param other     Other vector.
    constexpr element_t dot(const actual_t& other) const noexcept {
        element_t result = 0;
        for (std::size_t i = 0; i < super_t::get_dimensions(); ++i) {
            result += data[i] * other[i];
        }
        return result;
    }

    /// Tests whether this vector is orthogonal to the other.
    /// The zero vector is orthogonal to every vector.
    /// @param other     Vector to test against.
    bool is_orthogonal_to(actual_t other) const {
        // normalization required for large absolute differences in vector lengths
        return abs(get_fast_normalized().dot(other.fast_normalize())) <= precision_high<element_t>();
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

// is arithmetic type =============================================================================================== //

/// Helper struct to check if a given type is derived from Arithmetic.
template<class T>
struct ArithmeticDetector {
    NOTF_CREATE_TYPE_DETECTOR(actual_t);
    NOTF_CREATE_TYPE_DETECTOR(component_t);
    NOTF_CREATE_FIELD_DETECTOR(dimensions);
    static constexpr bool check() noexcept {
        if constexpr (_has_actual_t_v<T> && _has_component_t_v<T> && _has_dimensions_v<T>) {
            return std::is_base_of_v<Arithmetic<typename T::actual_t, typename T::component_t, T::dimensions>, T>;
        } else {
            return false;
        }
    }
};

} // namespace detail

/// Checks if a given type is a subclass of notf::detail::Arithmetic.
template<class T>
static constexpr bool is_arithmetic_type_v = detail::ArithmeticDetector<T>::check();

NOTF_CLOSE_NAMESPACE
