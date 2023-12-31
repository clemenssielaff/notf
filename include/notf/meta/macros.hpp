#pragma once

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// compatibility ==================================================================================================== //

#ifdef NOTF_MSVC
// https://blogs.msdn.microsoft.com/vcblog/2018/07/06/msvc-preprocessor-progress-towards-conformance/
#ifndef _MSVC_TRADITIONAL
#define _MSVC_TRADITIONAL 1
#endif
#endif

// utilities ======================================================================================================== //

/// No-op.
#define NOTF_NOOP ((void)0)

/// Interprets an expanded macro as a string.
#define NOTF_STR(x) #x

/// Takes two macros and concatenates them without whitespace in between.
#if defined NOTF_MSVC && _MSVC_TRADITIONAL
#define _notf_CONCAT(x, y) x##y
#define NOTF_CONCAT(x, y) _notf_CONCAT(x, y)
#else
#define NOTF_CONCAT(x, y) x##y
#endif

/// Expands a macro inside another macro.
/// Use for `NOTF_DEFER(NOTF_STR, expanded) and others
#if defined NOTF_MSVC && _MSVC_TRADITIONAL
#define _notf_ECHO(...) __VA_ARGS__
#define _notf_CALL(f, args) f args
#define NOTF_DEFER(f, ...) _notf_CALL(_notf_ECHO(f), _notf_ECHO((__VA_ARGS__)))
#else
#define NOTF_DEFER(f, ...) f(__VA_ARGS__)
#endif

/// Ignores variadic arguments when called with:
///     NOTF_IGNORE_VARIADIC(expr, , ##__VA_ARGS__) // expands to (expr)
#define NOTF_IGNORE_VARIADIC(h, ...) (h)

// compiler directives ============================================================================================== //

/// Compiler builtin detection, as described in:
///     https://clang.llvm.org/docs/LanguageExtensions.html#has-builtin
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/// Compiler attribute detection, as described in:
///     https://clang.llvm.org/docs/LanguageExtensions.html#has-cpp-attribute
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) (0)
#endif

/// Tells the compiler that a given statement is likely to be evaluated to true.
#if __has_builtin(__builtin_expect)
#define NOTF_LIKELY(x) __builtin_expect(!!(x), 1)
#define NOTF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NOTF_LIKELY(x) (!!(x))
#define NOTF_UNLIKELY(x) (!!(x))
#endif

/// NOTF_NODISCARD attribute to make sure that the return value of a function is not immediately discarded.
#if __has_cpp_attribute(nodiscard)
#define NOTF_NODISCARD [[nodiscard]]
#elif __has_cpp_attribute(gnu::warn_unused_result)
#define NOTF_NODISCARD [[gnu::warn_unused_result]]
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define NOTF_NODISCARD _Check_return_
#elif defined(__clang__) || defined(__GNUC__)
#define NOTF_NODISCARD __attribute__((__warn_unused_result__))
#else
#define NOTF_NODISCARD
#endif

/// Signifies that a value is (probably) unused and you don't want warnings about it.
#if __has_cpp_attribute(maybe_unused)
#define NOTF_UNUSED [[maybe_unused]]
#elif defined __has_cpp_attribute(gnu::unused)
#define NOTF_UNUSED [[gnu::unused]]
#else
#define NOTF_UNUSED
#endif

/// Signifies that the function will not return control flow back to the caller.
#if __has_cpp_attribute(noreturn)
#define NOTF_NORETURN [[noreturn]]
#else
#define NOTF_NORETURN
#endif

/// Indicates that a specific point in the program cannot be reached, even if the compiler might otherwise think it can.
#if __has_builtin(__builtin_unreachable)
#define NOTF_UNREACHABLE __builtin_unreachable()
#else
#define NOTF_UNREACHABLE
#endif

/// Explicit switch-case fallthrough.
#define NOTF_FALLTHROUGH [[fallthrough]]

/// Pragma support for macros.
#ifdef NOTF_MSVC
#define NOTF_PRAGMA(x) __pragma(x)
#else
#define NOTF_PRAGMA(x) _Pragma(x)
#endif

/// Not undefined because boolean reasoning is hard.
#define NOTF_THROWS_IF(x) noexcept(!(x))
#define NOTF_THROWS noexcept(false)

// object definition ================================================================================================ //

/// Convenience macro to disable the construction of automatic copy- and assign methods.
/// Implicitly disables move constructor/assignment methods as well, although you can define them yourself if you want.
#define NOTF_NO_COPY_OR_ASSIGN(Type) \
    Type(const Type&) = delete;      \
    void operator=(const Type&) = delete

/// Forbids the allocation on the heap of a given type.
#define NOTF_NO_HEAP_ALLOCATION(Type)      \
    void* operator new(size_t) = delete;   \
    void* operator new[](size_t) = delete; \
    void operator delete(void*) = delete;  \
    void operator delete[](void*) = delete

// forwards ========================================================================================================= //

// TODO: these NOTF_DECLARE_* macros are getting ridiculous

#define NOTF_DECLARE_SHARED_POINTERS_ONLY(Type)         \
    using Type##Ptr = std::shared_ptr<Type>;            \
    using Type##ConstPtr = std::shared_ptr<const Type>; \
    using Type##WeakPtr = std::weak_ptr<Type>;          \
    using Type##WeakConstPtr = std::weak_ptr<const Type>

/// Convenience macro to define shared pointer types for a given type.
#define NOTF_DECLARE_SHARED_POINTERS(Tag, Type)         \
    Tag Type;                                           \
    using Type##Ptr = std::shared_ptr<Type>;            \
    using Type##ConstPtr = std::shared_ptr<const Type>; \
    using Type##WeakPtr = std::weak_ptr<Type>;          \
    using Type##WeakConstPtr = std::weak_ptr<const Type>

/// Convenience macro to define unique pointer types for a given type.
#define NOTF_DECLARE_UNIQUE_POINTERS(Tag, Type) \
    Tag Type;                                   \
    using Type##Ptr = std::unique_ptr<Type>;    \
    using Type##ConstPtr = std::unique_ptr<const Type>

/// Convenience macro to define shared pointer alias types for a given type.
#define NOTF_DECLARE_SHARED_ALIAS_POINTERS(Type, Alias)  \
    using Alias##Ptr = std::shared_ptr<Type>;            \
    using Alias##ConstPtr = std::shared_ptr<const Type>; \
    using Alias##WeakPtr = std::weak_ptr<Type>;          \
    using Alias##WeakConstPtr = std::weak_ptr<const Type>

/// Convenience macro to define unique pointer alias types for a given type.
#define NOTF_DECLARE_UNIQUE_ALIAS_POINTERS(Type, Alias) \
    using Alias##Ptr = std::unique_ptr<Type>;           \
    using Alias##ConstPtr = std::unique_ptr<const Type>

/// Convenience macro to define shared pointer types for a given templated type with one variadic template argument.
#define NOTF_DECLARE_SHARED_POINTERS_VAR_TEMPLATE1(Tag, Type)  \
    template<class...>                                         \
    Tag Type;                                                  \
    template<class... Ts>                                      \
    using Type##Ptr = std::shared_ptr<Type<Ts...>>;            \
    template<class... Ts>                                      \
    using Type##ConstPtr = std::shared_ptr<const Type<Ts...>>; \
    template<class... Ts>                                      \
    using Type##WeakPtr = std::weak_ptr<Type<Ts...>>;          \
    template<class... Ts>                                      \
    using Type##WeakConstPtr = std::weak_ptr<const Type<Ts...>>

/// Convenience macro to define unique pointer types for a given templated type with one variadic template argument.
#define NOTF_DECLARE_UNIQUE_POINTERS_VAR_TEMPLATE1(Tag, Type) \
    template<class...>                                        \
    Tag Type;                                                 \
    template<class... Ts>                                     \
    using Type##Ptr = std::unique_ptr<Type<Ts...>>;           \
    template<class... Ts>                                     \
    using Type##ConstPtr = std::unique_ptr<const Type<Ts...>>

/// Convenience macro to define shared pointer types for a given templated type with one template argument.
#define NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(Tag, Type)  \
    template<class>                                        \
    Tag Type;                                              \
    template<class T>                                      \
    using Type##Ptr = std::shared_ptr<Type<T>>;            \
    template<class T>                                      \
    using Type##ConstPtr = std::shared_ptr<const Type<T>>; \
    template<class T>                                      \
    using Type##WeakPtr = std::weak_ptr<Type<T>>;          \
    template<class T>                                      \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T>>

/// Convenience macro to define unique pointer types for a given templated type with one template argument.
#define NOTF_DECLARE_UNIQUE_POINTERS_TEMPLATE1(Tag, Type) \
    template<class>                                       \
    Tag Type;                                             \
    template<class T>                                     \
    using Type##Ptr = std::unique_ptr<Type<T>>;           \
    template<class T>                                     \
    using Type##ConstPtr = std::unique_ptr<const Type<T>>

/// Convenience macro to define shared pointer types for a given templated type with two template arguments.
#define NOTF_DECLARE_SHARED_POINTERS_TEMPLATE2(Tag, Type)     \
    template<class, class>                                    \
    Tag Type;                                                 \
    template<class T, class U>                                \
    using Type##Ptr = std::shared_ptr<Type<T, U>>;            \
    template<class T, class U>                                \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U>>; \
    template<class T, class U>                                \
    using Type##WeakPtr = std::weak_ptr<Type<T, U>>;          \
    template<class T, class U>                                \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U>>

/// Convenience macro to define unique pointer types for a given templated type with two template arguments.
#define NOTF_DECLARE_UNIQUE_POINTERS_TEMPLATE2(Tag, Type) \
    template<class, class>                                \
    Tag Type;                                             \
    template<class T, class U>                            \
    using Type##Ptr = std::unique_ptr<Type<T, U>>;        \
    template<class T, class U>                            \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U>>

/// Convenience macro to define shared pointer types for a given templated type with three template arguments.
#define NOTF_DECLARE_SHARED_POINTERS_TEMPLATE3(Tag, Type)        \
    template<class, class, class>                                \
    Tag Type;                                                    \
    template<class T, class U, class V>                          \
    using Type##Ptr = std::shared_ptr<Type<T, U, V>>;            \
    template<class T, class U, class V>                          \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U, V>>; \
    template<class T, class U, class V>                          \
    using Type##WeakPtr = std::weak_ptr<Type<T, U, V>>;          \
    template<class T, class U, class V>                          \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U, V>>

/// Convenience macro to define unique pointer types for a given templated type with three template arguments.
#define NOTF_DECLARE_UNIQUE_POINTERS_TEMPLATE3(Tag, Type) \
    template<class, class, class>                         \
    Tag Type;                                             \
    template<class T, class U, class V>                   \
    using Type##Ptr = std::unique_ptr<Type<T, U, V>>;     \
    template<class T, class U, class V>                   \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U, V>>

/// Convenience macro to define shared pointer types for a given templated type with four template arguments.
#define NOTF_DECLARE_SHARED_POINTERS_TEMPLATE4(Tag, Type)           \
    template<class, class, class, class>                            \
    Tag Type;                                                       \
    template<class T, class U, class V, class W>                    \
    using Type##Ptr = std::shared_ptr<Type<T, U, V, W>>;            \
    template<class T, class U, class V, class W>                    \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U, V, W>>; \
    template<class T, class U, class V, class W>                    \
    using Type##WeakPtr = std::weak_ptr<Type<T, U, V, W>>;          \
    template<class T, class U, class V, class W>                    \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U, V, W>>

/// Convenience macro to define unique pointer types for a given templated type with four template arguments.
#define NOTF_DECLARE_UNIQUE_POINTERS_TEMPLATE4(Tag, Type) \
    template<class, class, class, class>                  \
    Tag Type;                                             \
    template<class T, class U, class V, class W>          \
    using Type##Ptr = std::unique_ptr<Type<T, U, V, W>>;  \
    template<class T, class U, class V, class W>          \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U, V, W>>

// raii ============================================================================================================= //

/// Uniquely named RAII guard object.
#define NOTF_GUARD(f) const auto NOTF_DEFER(NOTF_CONCAT, __notf__guard_, __COUNTER__) = f

// forward const as mutable ========================================================================================= //

/// Convenience macro for easy const-correctness.
/// Example:
///     const Foo& get_foo(size_t amount) const;
///     Foo& get_foo(size_t amount) { return NOTF_FORWARD_CONST_AS_MUTABLE(get_foo(amount)); }
///
#define NOTF_FORWARD_CONST_AS_MUTABLE(function) \
    const_cast<decltype(function)>(             \
        const_cast<std::add_pointer_t<std::add_const_t<std::remove_reference_t<decltype(*this)>>>>(this)->function)

// macro overloading ================================================================================================ //

/// Macro overloading for up to 16 parameters (from http://stackoverflow.com/a/30566098)
/// Use to define "MY_MACRO" as follows:
///
///     #define MY_MACRO(...) NOTF_OVERLOADED_MACRO(MY_MACRO, __VA_ARGS__)
///     #define MY_MACRO1(x) <implementation>
///     #define MY_MACRO2(x, y) <implementation>
///     #define MY_MACRO3(x, y, z) <implementation>
///
/// If you need more than 16, just append the numbers to `_notf_ARG_PATTERN_MATCH`
/// and prepend them in reverse to `_notf_COUNT_ARGS`.
#define NOTF_OVERLOADED_MACRO(macro_name, ...) _notf_OVERLOAD(macro_name, _notf_COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define _notf_OVERLOAD_EXPAND(macro_name, number_of_args) macro_name##number_of_args
#define _notf_OVERLOAD(macro_name, number_of_args) _notf_OVERLOAD_EXPAND(macro_name, number_of_args)
#define _notf_ARG_PATTERN_MATCH(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, n, ...) n
#define _notf_COUNT_ARGS(...) \
    _notf_ARG_PATTERN_MATCH(__VA_ARGS__, 16, 15, 14, 13, 13, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

NOTF_CLOSE_NAMESPACE
