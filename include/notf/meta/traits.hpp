#pragma once

#include <type_traits>

#include "./meta.hpp"

NOTF_OPEN_META_NAMESPACE

// traits =========================================================================================================== //

/// Checks if T is any of the variadic types.
template<class T, class... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value> {
};

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
///         static_assert(always_false<UNSUPPORTED_TYPE>{}, "Cannot convert Vector4d ...");
///     }
///
template<class T>
struct always_false : std::false_type {};

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

NOTF_CLOSE_META_NAMESPACE
