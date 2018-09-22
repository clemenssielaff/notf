#pragma once

#include <type_traits>
#include <typeinfo>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// traits =========================================================================================================== //

/// Checks if all expressions are true.
///     if(all(a, !b, c < d)){ // is equal to ```if(a && !b && c < d){```
template<class... Ts>
constexpr bool all(Ts... expressions)
{
    return (... && expressions);
}

/// Checks if T is any of the variadic types.
template<class T, class... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed_v<T> == std::is_signed_v<U>> {};

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
template<class T>
constexpr bool always_false_v = always_false<T>::value;

/// Always true, if T is a valid type.
template<class T>
struct always_true : std::true_type {};
template<class T>
constexpr bool always_true_v = always_true<T>::value;

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

/// Is only a valid expression if the given type exists.
/// Can be used to check whether a template type has a certain nested type:
///
///     template <typename T, typename = void>
///     constexpr bool has_my_type = false;
///     template <typename T>
///     constexpr bool has_my_type<T, decltype(check_is_type<typename T::has_my_type>(), void())> = true;
///
template<class T>
constexpr void check_is_type()
{
    static_cast<void>(typeid(T));
}

/// Default factory can be used to (not really) instantiate types within a decltype expression, even if their default
/// constructor is deleted.
/// Without:
///     struct Foo{
///         Foo() = delete;
///     };
///     void overloaded(int) {}
///     void overloaded(Foo) {}
///
///     template<class T>
///     using can_be_overloaded = decltype(overloaded(T{}));
///     using foo_t = can_be_overloaded<Foo>; // ERROR: use of deleted function 'Foo::Foo()'
///
/// And with:
///     template<class T>
///     using can_be_overloaded = decltype(overloaded(default_factory<T>{}()));
///     using foo_t = can_be_overloaded<Foo>; // OKAY
///
template<class T>
struct default_factory {
    T operator()() const { return T{}; }
};

NOTF_CLOSE_NAMESPACE
