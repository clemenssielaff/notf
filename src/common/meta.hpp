#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

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
#else
#error Unknown compiler detected (searching for __clang__, _MSC_VER and __GNUC__)
#endif
#endif
#endif

//====================================================================================================================//

/// OS detection
#ifdef __linux__
#define NOTF_LINUX
#else
#ifdef _WIN32
#define NOTF_WINDOWS
#else
#error Unknown operating system detected (searching for __linux__ and _WIN32)
#endif
#endif

//====================================================================================================================//

/// Function name macro to use for logging and exceptions.
#ifdef NOTF_LOG_PRETTY_FUNCTION
#ifdef NOTF_CLANG
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#ifdef NOTF_MSVC
#define NOTF_CURRENT_FUNCTION __FUNCTION__
#else
#ifdef NOTF_GCC
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif
#endif
#endif
#else
#define NOTF_CURRENT_FUNCTION __func__
#endif

//====================================================================================================================//

/// Pragma support for macros.
#ifdef NOTF_MSVC
#define NOTF_PRAGMA(x) __pragma(x)
#else
#define NOTF_PRAGMA(x) _Pragma(x)
#endif

/// Nothing happening here.
#define NOTF_NOOP ((void)0)

/// Compiler attribute detection, as described in:
///     https://clang.llvm.org/docs/LanguageExtensions.html#has-cpp-attribute
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) NOTF_NOOP // Compatibility with non-clang compilers.
#endif

/// NOTF_NODISCARD attribute to make sure that the return value of a function is not immediately discarded.
/// Example:
///     NOTF_NODISCARD int guard() { ...
///     class NOTF_NODISCARD Guard { ...
#if __has_cpp_attribute(nodiscard)
#define NOTF_NODISCARD [[nodiscard]]
#elif __has_cpp_attribute(gnu::warn_unused_result)
#define NOTF_NODISCARD [[gnu::warn_unused_result]]
#else
#define NOTF_NODISCARD
#endif

/// Signifies that a value is (probably) unused and you don't want warnings about it.
/// Example:
///     NOTF_UNUSED int answer = 42;
#if __has_cpp_attribute(maybe_unused)
#define NOTF_UNUSED [[maybe_unused]]
#elif __has_cpp_attribute(gnu::unused)
#define NOTF_UNUSED [[gnu::unused]]
#else
#define NOTF_UNUSED
#endif

/// Signifies that the function will not return control flow back to the caller.
#define NOTF_NORETURN [[noreturn]]

/// Tells the compiler that a given statement is likely to be evaluated to true.
#ifdef __GNUC__
#define NOTF_LIKELY(x) __builtin_expect(!!(x), 1)
#define NOTF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NOTF_LIKELY(x) (!!(x))
#define NOTF_UNLIKELY(x) (!!(x))
#endif

//====================================================================================================================//

/// Interprets an expanded macro as a string.
#define NOTF_STR(x) #x

/// Takes two macros and concatenates them without whitespace in between.
#define NOTF_CONCAT(x, y) x##y

/// Expands a macro inside another macro.
/// Use for `NOTF_DEFER(NOTF_STR, expanded) and others
#define NOTF_DEFER(f, ...) f(__VA_ARGS__)

//====================================================================================================================//

/// Convienience macros to temporarely disable a single warning.
/// NOTF_DISABLE_WARNING("switch-enum")
///     ...
/// NOTF_ENABLE_WARNINGS
#define NOTF_DISABLE_WARNING_STR_(x) GCC diagnostic ignored "-W" x
#define NOTF_DISABLE_WARNING(x)          \
    NOTF_PRAGMA("clang diagnostic push") \
    NOTF_PRAGMA(NOTF_DEFER(NOTF_STR, NOTF_DISABLE_WARNING_STR_(x)))
#define NOTF_ENABLE_WARNINGS NOTF_PRAGMA("clang diagnostic pop")

/// Throw a compiler warning or error with some additional information.
#define NOTF_COMPILER_WARNING(x) \
    NOTF_PRAGMA(NOTF_DEFER(NOTF_STR, warning "(warning) on line " NOTF_DEFER(NOTF_STR, __LINE__) ": " x))

//====================================================================================================================//

/// std namespace injections
namespace std {

#ifndef NOTF_CPP14

template<bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

#endif

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

/// Convenience macro to disable the construction of automatic copy- and assign methods.
/// Implicitly disables move constructor/assignment methods as well, although you can define them yourself if you want.
#define NOTF_NO_COPY_OR_ASSIGN(Type) \
    Type(const Type&) = delete;      \
    void operator=(const Type&) = delete;

/// Forbids the allocation on the heap of a given type.
#define NOTF_NO_HEAP_ALLOCATION(Type)      \
    void* operator new(size_t) = delete;   \
    void* operator new[](size_t) = delete; \
    void operator delete(void*) = delete;  \
    void operator delete[](void*) = delete;

/// Convenience macro to define shared pointer types for a given type.
#define NOTF_DEFINE_SHARED_POINTERS(Tag, Type) \
    Tag Type;                                  \
    using Type##Ptr = std::shared_ptr<Type>;   \
    using Type##ConstPtr = std::shared_ptr<const Type>

/// Convenience macro to define unique pointer types for a given type.
#define NOTF_DEFINE_UNIQUE_POINTERS(Tag, Type) \
    Tag Type;                                  \
    using Type##Ptr = std::unique_ptr<Type>;   \
    using Type##ConstPtr = std::unique_ptr<const Type>

/// Private access type template.
/// Used for finer grained friend control and is compiled away completely (if you should worry).
#define NOTF_ACCESS_TYPES(...)                                                          \
    template<typename T, typename = std::enable_if_t<is_one_of<T, __VA_ARGS__>::value>> \
    class Access;

//====================================================================================================================//

/// Opens the NoTF namespace.
/// This macro can later be used to implement namespace versioning.
#define NOTF_OPEN_NAMESPACE namespace notf {

/// For visual balance with NOTF_OPEN_NAMESPACE.
#define NOTF_CLOSE_NAMESPACE }

/// Use the versioned namespace.
#define NOTF_USING_NAMESPACE using namespace notf;

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

using uchar = unsigned char;

//====================================================================================================================//

/// Checks if T is any of the variadic types.
template<typename T, typename... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

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

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value> {
};

//====================================================================================================================//

/// Constexpr to use an enum class value as a numeric value.
/// From "Effective Modern C++ by Scott Mayers': Item #10.
template<typename Enum, typename = std::enable_if_t<std::is_enum<Enum>::value>>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

/// Converts any pointer to the equivalent integer representation.
template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
constexpr std::uintptr_t to_number(const T ptr) noexcept
{
    return reinterpret_cast<std::uintptr_t>(ptr);
}

//====================================================================================================================//

/// Defined not because boolean reasoning is not hard.
#define NOTF_THROWS_IF(x) noexcept(!(x))
#define NOTF_THROWS noexcept(false)

/// Useful for cases like `NOTF_THROWS_IF(is_debug_build())`.
#ifdef NOTF_DEBUG
constexpr bool is_debug_build() noexcept { return true; }
#else
constexpr bool is_debug_build() noexcept { return false; }
#endif

//====================================================================================================================//

namespace detail {

/// Simple `new` forward to allow the creation of an instance from a protected or private constructor.
template<typename T, typename... Args>
inline T* make_new_enabler(Args&&... args)
{
    return new T(std::forward<Args>(args)...);
}

/// Helper struct to allow `std::make_shared` to work with protected or private constructors.
/// from:
///     https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
template<typename T>
struct make_shared_enabler : public T {
    template<typename... Args>
    make_shared_enabler(Args&&... args) : T(std::forward<Args>(args)...)
    {}
};

} // namespace detail

/// Put into the class body to allow the use of NOTF_MAKE_SHARED_FROM_PRIVATE
#define NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE           \
    template<typename _T, typename... _Args>         \
    friend _T* detail::make_new_enabler(_Args&&...); \
    template<typename>                               \
    friend struct detail::make_shared_enabler;

/// I found that debugging a `detail::make_shared_enable<T>` instance is not as reliable (members seem to be missing?)
/// as when you create a shared pointer from a raw pointer. Therefore, NOTF_MAKE_SHARED_FROM_PRIVATE works differently
/// in debug and release mode.
#ifdef NOTF_DEBUG
#define NOTF_MAKE_SHARED_FROM_PRIVATE(T, ...) std::shared_ptr<T>(detail::make_new_enabler<T>(__VA_ARGS__))
#define NOTF_MAKE_UNIQUE_FROM_PRIVATE(T, ...) std::unique_ptr<T>(detail::make_new_enabler<T>(__VA_ARGS__))
#else
#define NOTF_MAKE_SHARED_FROM_PRIVATE(T, ...) std::make_shared<detail::make_shared_enabler<T>>(__VA_ARGS__);
#define NOTF_MAKE_UNIQUE_FROM_PRIVATE(T, ...) std::make_unique<detail::make_shared_enabler<T>>(__VA_ARGS__);
#endif

//====================================================================================================================//

/// Extracts the last part of a pathname at compile time.
/// @param input     Path to investigate.
/// @param delimiter Delimiter used to separate path elements.
/// @return          Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return
///                  "file.cpp".
constexpr const char* basename(const char* input, const char delimiter = '/')
{
    size_t last_occurrence = 0;
    for (size_t offset = 0; input[offset]; ++offset) {
        if (input[offset] == delimiter) {
            last_occurrence = offset + 1;
        }
    }
    return &input[last_occurrence];
}

NOTF_CLOSE_NAMESPACE
