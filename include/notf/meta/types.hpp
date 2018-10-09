#pragma once

#include <type_traits>
#include <typeinfo>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// traits =========================================================================================================== //

/// Explicit None type.
struct None {
    bool operator==(const None&) const { return true; }
    bool operator<(const None&) const { return false; }
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
template<class... Ts>
struct always_false : std::false_type {};
template<class... Ts>
static constexpr const bool always_false_v = always_false<Ts...>::value;

/// Always true, if T is a valid type.
template<class... Ts>
struct always_true : std::true_type {};
template<class... Ts>
static constexpr const bool always_true_v = always_true<Ts...>::value;

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
using empty_type = void * [0];

/// Checks if all expressions are true.
///     `if(all(a, !b, c < d))` is equal to `if(a && !b && c < d)`
template<class... Ts>
constexpr bool all(Ts... expressions)
{
    return (... && expressions);
}

/// Checks if any of the given expressions are true.
///     `if(all(a, !b, c < d))` is equal to `if(a || !b || c < d)`
template<class... Ts>
constexpr bool any(Ts... expressions)
{
    return (... || expressions);
}

/// Checks if T is any of the variadic types.
template<class T, class... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed_v<T> == std::is_signed_v<U>> {};

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

// ================================================================================================================== //

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

// ================================================================================================================== //

/// Constexpr to use an enum class value as a numeric value.
/// From "Effective Modern C++ by Scott Mayers': Item #10.
template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

/// Converts any pointer to the equivalent integer representation.
template<class T, class = std::enable_if_t<std::is_pointer_v<T>>>
constexpr std::uintptr_t to_number(T ptr) noexcept
{
    return reinterpret_cast<std::uintptr_t>(ptr);
}

NOTF_CLOSE_NAMESPACE
