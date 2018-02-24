#pragma once

#include <limits.h>
#include <limits>
#include <stddef.h>
#include <type_traits>

//====================================================================================================================//

/// Definitions for the various versions of C++.
/// Wherever possible, it is more precise to use feature testing macros described in:
///     http://en.cppreference.com/w/User:D41D8CD98F/feature_testing_macros

#ifndef __cplusplus
#error A C++ compiler is required!
#else

#if __cplusplus >= 199711L
#define NOTF_CPP97
#endif
#if __cplusplus >= 201103L
#define NOTF_CPP11
#endif
#if __cplusplus >= 201402L
#define NOTF_CPP14
#endif
#if __cplusplus >= 201703L
#define NOTF_CPP17
#endif

#endif

//====================================================================================================================//

/// Compiler detection.
#ifdef __clang__
#define NOTF_CLANG
#else
#ifdef _MSC_VER
#define NOTF_MSVC
#else
#ifdef __GNUC__
#define NOTF_GCC
#endif
#endif
#endif

//====================================================================================================================//

/// Compiler attribute detection, as described in:
///     https://clang.llvm.org/docs/LanguageExtensions.html#has-cpp-attribute
#ifndef __has_cpp_attribute      // Optional of course.
#define __has_cpp_attribute(x) 0 // Compatibility with non-clang compilers.
#endif

/// NODISCARD attribute to make sure that the return value of a function is not immediately discarded.
/// Example:
///     int NODISCARD guard() { ...
///     class NODISCARD Guard { ...
#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#elif __has_cpp_attribute(gnu::warn_unused_result)
#define NODISCARD [[gnu::warn_unused_result]]
#else
#define NODISCARD
#endif

/// Signifies that a value is (probably) unused and you don't want warnings about it.
/// Example:
///     UNUSED int answer = 42;
#if __has_cpp_attribute(maybe_unused)
#define UNUSED [[maybe_unused]]
#elif __has_cpp_attribute(gnu::unused)
#define UNUSED [[gnu::unused]]
#else
#define UNUSED
#endif

//====================================================================================================================//

/// std namespace injections
namespace std {

#ifndef NOTF_CPP17

/// Variadic logical AND metafunction
/// http://en.cppreference.com/w/cpp/types/conjunction
template<typename...>
struct conjunction : std::true_type {};
template<typename T>
struct conjunction<T> : T {};
template<typename T, typename... TList>
struct conjunction<T, TList...> : std::conditional<T::value, conjunction<TList...>, T>::type {};

/// Variadic logical OR metafunction
/// http://en.cppreference.com/w/cpp/types/disjunction
template<typename...>
struct disjunction : std::false_type {};
template<typename T>
struct disjunction<T> : T {};
template<typename T, typename... TList>
struct disjunction<T, TList...> : std::conditional<T::value, T, disjunction<TList...>>::type {};

/// Logical NOT metafunction.
/// http://en.cppreference.com/w/cpp/types/negation
template<typename T>
struct negation : std::integral_constant<bool, !T::value> {};

#endif

#ifndef __cpp_lib_void_t

/// Void type.
template<typename... Ts>
struct make_void {
    typedef void type;
};
template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

#endif

} // namespace std

//====================================================================================================================//

/// Checks if T is any of the variadic types.
template<typename T, typename... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

//====================================================================================================================//

#define ENABLE_IF_REAL(TYPE) \
    typename std::enable_if<std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true

#define DISABLE_IF_REAL(TYPE) \
    typename std::enable_if<!std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true

#define ENABLE_IF_INT(TYPE) \
    typename std::enable_if<std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true

#define DISABLE_IF_INT(TYPE) \
    typename std::enable_if<!std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true

#define ENABLE_IF_SAME(TYPE_A, TYPE_B)                                                                                 \
    typename std::enable_if<std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, \
                            bool>::type                                                                                \
        = true

#define ENABLE_IF_ARITHMETIC(TYPE) \
    typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type = true

#define ENABLE_IF_SUBCLASS(TYPE, PARENT) \
    typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true

//====================================================================================================================//

/// The `always_false` struct can be used to create a templated struct that will always evaluate to `false` when
/// used in a static_assert.
///
/// Imagine the following scenario: You have a scene graph and a fairly generic operation `calc` on some object class,
/// that you want to be performed in various spaces:
///
///     enum class Space {
///        WORLD,
///        LOCAL,
///        SCREEN,
///     };
///
/// Instead of passing a `Space` parameter at runtime, or cluttering the interface with `calc_in_world` and
/// `calc_in_local` etc. -operations, you want a template method: `calc<Space>()`. Then you can specialize the template
/// method for various spaces. If a space is *not* supported, you can just not specialize the method for it. The
/// original template method would look this this:
///
///     template<Space space>
///     float calc() {
///         static_assert(always_false<Space, space>{}, "Unsupported Space");
///     }
///
/// And specializations like this:
///
///     template<>
///     float calc<Space:WORLD>() { return 0; }
///
/// You can now call supported specializations, but unsupported ones will fail at runtime:
///
///     calc<Space::WORLD>();  // produces 0
///     calc<Space::SCREEN>(); // error! static assert: "Unsupported Space"
///
template<typename T, T val>
struct always_false : std::false_type {};

/// Like always_false, but taking a single type only.
/// This way you can define error overloads that differ by type only:
///
///     static void convert(const Vector4d& in, std::array<float, 4>& out)
///     {
///         ...
///     }
///
///     template <typename UNSUPPORTED_TYPE>
///     static void convert(const Vector4d& in, UNSUPPORTED_TYPE& out)
///     {
///         static_assert(always_false_t<T>{}, "Cannot convert a Vector4d into template type T");
///     }
///
template<typename T>
struct always_false_t : std::false_type {};

//====================================================================================================================//

/// Convenience macro to disable the construction of automatic copy- and assign methods.
/// Implicitly disables move constructor/assignment methods as well, although you can define them yourself if you want.
#define DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&) = delete;        \
    void operator=(const Type&) = delete;

/// Forbids the allocation on the heap of a given type.
#define DISALLOW_HEAP_ALLOCATION(Type)      \
    void* operator new(size_t)    = delete; \
    void* operator new[](size_t)  = delete; \
    void operator delete(void*)   = delete; \
    void operator delete[](void*) = delete;

/// Convenience macro to define shared pointer types for a given type.
#define DEFINE_SHARED_POINTERS(Tag, Type)         \
    Tag Type;                                     \
    using Type##Ptr      = std::shared_ptr<Type>; \
    using Type##ConstPtr = std::shared_ptr<const Type>

/// Convenience macro to define unique pointer types for a given type.
#define DEFINE_UNIQUE_POINTERS(Tag, Type)         \
    Tag Type;                                     \
    using Type##Ptr      = std::unique_ptr<Type>; \
    using Type##ConstPtr = std::unique_ptr<const Type>

//====================================================================================================================//

/// Trait containing the type that has a higher numeric limits.
template<typename LEFT, typename RIGHT>
struct higher_type {
    using type = typename std::conditional<std::numeric_limits<LEFT>::max() <= std::numeric_limits<RIGHT>::max(), LEFT,
                                           RIGHT>::type;
};

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value> {
};

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<typename T>
constexpr size_t bitsizeof()
{
    return sizeof(T) * CHAR_BIT;
}

//====================================================================================================================//

/// Takes two macros and concatenates them without whitespace in between.
#define MACRO_CONCAT_(A, B) A##B
#define MACRO_CONCAT(A, B) MACRO_CONCAT_(A, B)

//====================================================================================================================//

/// Overload helper to use with std::variant in C++17.
/// For details see:
///     http://en.cppreference.com/w/cpp/utility/variant/visit
#ifdef NOTF_CPP17
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;
#endif

//====================================================================================================================//

/// Calling a const getter into a non-const getter.
/// Use like:
///     const Type& get() const { ... }
///     Type& get() { NONCONST_GETTER(get) }
#define NONCONST_GETTER(METHOD, ...)                                                                    \
    {                                                                                                   \
        using const_thisT = typename std::add_const<std::decay<decltype(*this)>::type>::type*;          \
        using returnT     = typename std::remove_const<                                                 \
            std::decay<decltype(const_cast<const_thisT>(this)->METHOD(__VA_ARGS__))>::type>::type&; \
        return const_cast<returnT>(const_cast<const_thisT>(this)->METHOD(__VA_ARGS__));                 \
    }

//====================================================================================================================//

/// Function name macro to use for logging and exceptions.
#ifdef NOTF_LOG_PRETTY_FUNCTION
#ifdef NOTF_CLANG
#define NOTF_FUNCTION __PRETTY_FUNCTION__
#else
#ifdef NOTF_MSVC
#define NOTF_FUNCTION __FUNCTION__
#else
#ifdef NOTF_GCC
#define NOTF_FUNCTION __PRETTY_FUNCTION__
#endif
#endif
#endif
#else
#define NOTF_FUNCTION __func__
#endif

//====================================================================================================================//
