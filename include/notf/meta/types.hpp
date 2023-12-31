#pragma once

#include <cstdint>
#include <typeinfo>
#include <utility>

#include <array>

#include "notf/meta/assert.hpp"
#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// short unsigned integer names ===================================================================================== //

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

// is_number ======================================================================================================== //

/// Checks whether T is an integral or real numeric type.
template<class T>
struct is_numeric : std::disjunction<std::is_integral<T>, std::is_floating_point<T>> {};
template<class T>
inline constexpr const bool is_numeric_v = is_numeric<T>::value;

// is un/bounded array ============================================================================================== //

/// Checks if T is a bounded array type.
/// This will be part of C++20
template<class T>
struct is_bounded_array : std::false_type {};
template<class T, std::size_t N>
struct is_bounded_array<T[N]> : std::true_type {};
template<class T>
inline constexpr const bool is_bounded_array_v = is_bounded_array<T>::value;

/// Checks if T is an unbounded array type.
/// This will be part of C++20
template<class T>
struct is_unbounded_array : std::false_type {};
template<class T>
struct is_unbounded_array<T[]> : std::true_type {};
template<class T>
inline constexpr const bool is_unbounded_array_v = is_unbounded_array<T>::value;

// traits =========================================================================================================== //

/// Explicit None type.
struct None {
    bool operator==(const None&) const { return true; }
    bool operator<(const None&) const { return false; }
};

/// Explicit All type, similar to None, but while None denotes "no data" All says "there is a single piece of data, but
/// I don't care about the type (and am probably going to ignore it)". Used in reactive subscribers, for example.
struct All {
    bool operator==(const All&) const { return true; }
    bool operator<(const All&) const { return false; }
};

/// Sometimes we want an "explicit" yes or no or an implicit "default".
enum class Tristate {
    Default = -1,
    False = 0,
    True = 1,
};

/// Type template to ensure that a template argument does not participate in type deduction.
/// Compare:
///
///     template <class T>
///     void multiply_each(std::vector<T>& vec, T factor) {
///         for (auto& item : vec){
///             item *= factor;
///         }
///     }
///     std::vector<long> vec{1, 2, 3};
///     multiply_each(vec, 5);  // error: no matching function call to
///                             // multipy_each(std::vector<long int>&, int)
///                             // note: template argument deduction/substitution failed:
///                             // note: deduced conflicting types for parameter `T` (`long int` and `int`)
///
/// with:
///
///     template <class T>
///     void multiply_each(std::vector<T>& vec, identity_t<T> factor)
///     {
///         for (auto& item : vec){
///             item *= factor;
///         }
///     }
///     std::vector<long> vec{1, 2, 3};
///     multiply_each(vec, 5);  // works
///
template<class T>
struct identity {
    using type = T;
};
template<class T>
using identity_t = typename identity<T>::type;

/// Like identity, but with an additional index.
/// Use to convert an index from a std::index_sequence into T.
template<class T, size_t>
struct identity_index {
    using type = T;
};
template<class T, size_t I>
using identity_index_t = typename identity_index<T, I>::type;

/// Produces the given value, no matter what index is given as the second template argument.
template<class T, size_t = 0>
constexpr T identity_func(const T& value) {
    return value;
}

/// The `always_false` trait will always evaluate to `false` when used in a static_assert.
/// This way you can define error overloads that differ by type only:
///
///     static void convert(const Vector4d& in, std::array<float, 4>& out)
///     {
///         ...
///     }
///
///     template <class UNSUPPORTED_TYPE>
///     static void convert(const Vector4d& in, UNSUPPORTED_TYPE& out)
///     {
///         static_assert(always_false_v<UNSUPPORTED_TYPE>, "Cannot convert Vector4d ...");
///     }
///
template<class... Ts>
struct always_false : std::false_type {};
template<class... Ts>
inline constexpr const bool always_false_v = always_false<Ts...>::value;

/// Always true, if T is a valid type.
template<class... Ts>
struct always_true : std::true_type {};
template<class... Ts>
inline constexpr const bool always_true_v = always_true<Ts...>::value;

/// Use in place of a type if you don't want the type to take up any space.
/// This is actually not "valid" C++ but all compilers (that I tested) allow it without problems.
/// Read all about it:
///     - https://gcc.gnu.org/onlinedocs/gcc/Zero-Length.html
///     - https://herbsutter.com/2009/09/02/when-is-a-zero-length-array-okay/
///
/// Fact is:
///
///     using none_t = std::tuple<>;
///     using zeroarray_t = void*[0];
///     struct empty_t {};
///
///     using other_t = double;
///
///     struct OtherAndNone {
///       other_t other;
///       none_t none;
///     };
///     struct OtherAndZeroArray {
///       other_t other;
///       zeroarray_t zeroarray;
///     };
///     struct OtherAndEmpty {
///       other_t other;
///       empty_t empty;
///     };
///
///     sizeof(other_t)             // =  8
///     sizeof(none_t)              // =  1
///     sizeof(zeroarray_t)         // =  0
///     sizeof(empty_t)             // =  1
///     sizeof(OtherAndNone)        // = 16
///     sizeof(OtherAndZeroArray)   // =  8
///     sizeof(OtherAndEmpty)       // = 16
///
using empty_type = void* [0];

/// Checks if all expressions are true.
///     `if(all(a, !b, c < d))` is equal to `if(a && !b && c < d)`
template<class... Ts>
constexpr bool all(Ts... expressions) {
    return (... && expressions);
}

/// Checks if any of the given expressions are true.
///     `if(all(a, !b, c < d))` is equal to `if(a || !b || c < d)`
template<class... Ts>
constexpr bool any(Ts... expressions) {
    return (... || expressions);
}

/// Checks if T is any of the variadic types.
template<class T, class... Ts>
inline constexpr const bool is_one_of_v = std::disjunction_v<std::is_same<T, Ts>...>;

/// Checks if T is derived from any of the variadic types.
template<class T, class... Ts>
inline constexpr const bool is_derived_from_one_of_v = std::disjunction_v<std::is_base_of<Ts, T>...>;

/// Checks if all Ts are the same type as T.
template<class T, class... Ts>
inline constexpr const bool all_of_type_v = std::conjunction_v<std::is_same<T, Ts>...>;

/// Checks if all Ts are convertible to T.
template<class T, class... Ts>
inline constexpr const bool all_convertible_to_v = std::conjunction_v<std::is_convertible<T, Ts>...>;

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed_v<T> == std::is_signed_v<U>> {};
template<class T, class U>
inline constexpr const bool is_same_signedness_v = is_same_signedness<T, U>::value;

/// Compile-time check whether T is both trivial and standard-layout.
template<class T>
struct is_pod
    : public std::integral_constant<bool, std::conjunction_v<std::is_trivial<T>, std::is_standard_layout<T>>> {};
template<class T>
inline constexpr const bool is_pod_v = is_pod<T>::value;

// accessors ======================================================================================================== //

/// "Special Access" type, that can be befriended by any class that wants to expose a subset of its private interface
/// to another. Note that the struct must be defined outside the class.
/// Use as follows:
///     friend struct Accesssor<ThisClass, FriendClass>;
///
template<class Accessed, class Granted>
struct Accessor {};

/// Structure that can be used to grant access to a "Tester" class, that only exist when the tests are built.
#ifdef NOTF_TEST
struct Tester {};
#endif

#ifdef NOTF_TEST
#define NOTF_ACCESS_TYPE(NOTF_MACRO_ARG1)                            \
    template<class NOTF_TEMPLATE_ARG1>                               \
    using AccessFor = Accessor<NOTF_MACRO_ARG1, NOTF_TEMPLATE_ARG1>; \
    friend AccessFor<Tester>
#else
#define NOTF_ACCESS_TYPE(NOTF_MACRO_ARG1) \
    template<class NOTF_TEMPLATE_ARG1>    \
    using AccessFor = Accessor<NOTF_MACRO_ARG1, NOTF_TEMPLATE_ARG1>
#endif

// to number ======================================================================================================== //

/// Constexpr to use an enum class value as a numeric value.
/// From "Effective Modern C++ by Scott Mayers': Item #10.
template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr auto to_number(Enum enumerator) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

/// Converts any pointer to the equivalent integer representation.
template<class T, class = std::enable_if_t<std::is_pointer_v<T>>>
constexpr std::uintptr_t to_number(T ptr) noexcept {
    return reinterpret_cast<std::uintptr_t>(ptr);
}
constexpr std::uintptr_t to_number(std::nullptr_t) noexcept { return 0; }

// auto list ======================================================================================================== //

/// Use to create a list of compatible objects to iterate through.
template<class Common, class... Args>
constexpr std::array<Common, sizeof...(Args)> auto_list(Args&&... args) {
    return {static_cast<Common>(std::forward<Args>(args))...};
}

// null result ====================================================================================================== //

/// @{
/// Produces a type-specific "null" value or void.
template<class T>
constexpr T null_result() noexcept {
    static_assert(std::is_nothrow_default_constructible<T>::value);
    return {};
}
template<>
inline void null_result<void>() noexcept {};
/// @}

// declval ========================================================================================================== //

#ifndef NOTF_COMPILER_HAS_DECLVAL
/// GCC seems to have a bug that causes the expression
///     decltype(delcval<T>())
/// to be evaluated at compile time, triggering a static assert in its definition of declval.
/// This is the definition supplied by cppreference (https://en.cppreference.com/w/cpp/utility/declval) which does the
/// same thing but doesn't assert.
template<class T>
extern typename std::add_rvalue_reference<T>::type declval() noexcept;
#else
using std::declval;
#endif

// ingest =========================================================================================================== //

template<class T>
class ingest;
template<class T>
struct is_ingest : std::false_type {};
template<class T>
struct is_ingest<ingest<T>> : std::true_type {};
template<class T>
inline constexpr const bool is_ingest_v = is_ingest<T>::value;

/// Normally, you cannot move from initializer lists, because the values can be anything including static variables that
/// may not be moved from to ensure program correctness.
/// This type can be used inside an initializer list, in order to differntiate at runtime, whether or not an object can
/// be moved or not.
/// This code is taken from
///     https://cpptruths.blogspot.com/2013/09/21-ways-of-passing-parameters-plus-one.html#inTidiom
/// where it is in turn reprinted from
///     https://codesynthesis.com/~boris/blog/2012/06/26/efficient-argument-passing-cxx11-part2
template<class T>
class ingest {

    // types ------------------------------------------------------------------------------------ //
private:
    // Support for implicit conversion via perfect forwarding.
    struct Converter {
        ~Converter() {
            if (is_finalized) { reinterpret_cast<T*>(&data)->~T(); }
        }
        std::aligned_storage_t<sizeof(T), alignof(T)> data;
        bool is_finalized = false;
    };

    // methods ---------------------------------------------------------------------------------- //
public:
    /// @{
    /// L-value constructor.
    /// Produces an `ingest` container that cannot be moved from.
    /// @param value    Value to ingest.
    constexpr ingest(T& value) noexcept : m_value(value), m_is_movable(false) {}
    constexpr ingest(const T& value) noexcept : m_value(value), m_is_movable(false) {}
    /// @}

    /// R-value constructor.
    /// Produces an `ingest` container that can be moved from.
    /// @param value    Value to ingest.
    constexpr ingest(T&& value) : m_value(value), m_is_movable(true) {}

    /// Implicit r-value conversion with perfect forwarding.
    /// Produces an `ingest` container that can be moved from.
    /// @param value    Value to ingest.
    template<class Value,
             class = std::enable_if_t<std::is_convertible_v<Value, T> && !is_ingest_v<std::remove_reference_t<Value>>>>
    ingest(Value&& value, Converter&& converter = Converter())
        : m_value(*new (&converter.data) T(std::forward<Value>(value))), m_is_movable(true) {
        converter.is_finalized = true;
    }

    /// Implicit conversion to const T&, this is always safe.
    constexpr operator const T&() const noexcept { return m_value; }

    /// Whether or not the value contained is movable or not.
    constexpr bool is_movable() const noexcept { return m_is_movable; }

    /// Creates a new value and returns it, either by moving the internal value out of this container or by copying it.
    constexpr T move() const {
        if (is_movable()) {
            return force_move();
        } else {
            return m_value;
        }
    }

    /// Forces a move.
    /// Use this when you are certain that the value is movable (for example, by calling `is_movable`) to save the
    /// construction of a new T in `move`.
    constexpr T&& force_move() const {
        NOTF_ASSERT(m_is_movable);
        return std::move(const_cast<T&>(m_value));
    }

    // fields ----------------------------------------------------------------------------------- //
private:
    /// Value to ingest, must be const because that's what an initializer list gives you.
    const T& m_value;

    /// Runtime flag identifying the value as movable or not.
    const bool m_is_movable;
};

NOTF_CLOSE_NAMESPACE
