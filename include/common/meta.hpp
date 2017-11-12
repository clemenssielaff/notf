#pragma once

#include <limits>
#include <type_traits>

/// @brief Tests if T is one of the following types.
template<typename T, typename...>
struct is_one_of : std::false_type {};
template<typename T, typename Head, typename... Rest>
struct is_one_of<T, Head, Rest...>
    : std::integral_constant<bool, std::is_same<T, Head>::value || is_one_of<T, Rest...>::value> {};

/// @brief Tests if all conditions are true.
template<typename T, typename...>
struct all_true : std::true_type {};
template<typename T, typename Head, typename... Rest>
struct all_true<T, Head, Rest...> : std::integral_constant<bool, Head::value && all_true<T, Rest...>::value> {};

/* At the beginning, the macros here used std::enable_if_t, but typedefs from those templates could not be properly
 * forward defined.
 * Therefore I came up with this method which is similar, also throws a compiler error if the check fails, but has a
 * slightly misleading error message (in gcc: "no type name 'type' in struct std::enable_if<false, bool>).
 *
 * Forward declare a template specialization like this:
 *
 *     template <typename value_t, FWD_ENABLE_IF_ARITHMETIC(value_t)>
 *     struct _Size2;
 *     using Size2i = _Size2<int, true>;
 *
 */

#define ENABLE_IF_REAL(TYPE) \
    typename std::enable_if<std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_REAL(TYPE) \
    typename std::enable_if<std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_REAL(TYPE) \
    typename std::enable_if<!std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_REAL(TYPE) \
    typename std::enable_if<!std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_INT(TYPE) \
    typename std::enable_if<std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_INT(TYPE) \
    typename std::enable_if<std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_INT(TYPE) \
    typename std::enable_if<!std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_INT(TYPE) \
    typename std::enable_if<!std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SAME(TYPE_A, TYPE_B)                                                                                 \
    typename std::enable_if<std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, \
                            bool>::type                                                                                \
        = true
#define FWD_ENABLE_IF_SAME(TYPE_A, TYPE_B)                                                                             \
    typename std::enable_if<std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, \
                            bool>::type

#define ENABLE_IF_SAME_ANY(TYPE_A, ...) \
    typename std::enable_if<is_same_any<typename std::decay<TYPE_A>::type, __VA_ARGS__>::value, bool>::type = true
#define FWD_ENABLE_IF_SAME_ANY(TYPE_A, ...) \
    typename std::enable_if<is_same_any<typename std::decay<TYPE_A>::type, __VA_ARGS__>::value, bool>::type

#define ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B)                                                                     \
    typename std::enable_if<                                                                                    \
        !std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type \
        = true
#define FWD_ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B) \
    typename std::enable_if<                    \
        !std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type

#define ENABLE_IF_ARITHMETIC(TYPE) \
    typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_ARITHMETIC(TYPE) \
    typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SUBCLASS(TYPE, PARENT) \
    typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS(TYPE, PARENT) \
    typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_SUBCLASS(TYPE, PARENT) \
    typename std::enable_if<!std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_SUBCLASS(TYPE, PARENT) \
    typename std::enable_if<!std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SUBCLASS_ANY(TYPE, ...) \
    typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS_ANY(TYPE, ...) \
    typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type

//====================================================================================================================//

/// @brief The `always_false` struct can be used to create a templated struct that will always evaluate to `false` when
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

/// @brief Like always_false, but taking a single type only.
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

/// @brief Convenience macro to disable the construction of automatic copy- and assign methods.
/// Implicitly disables move constructor/assignment methods as well, although you might define them yourself if you want
/// to.
#define DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&) = delete;        \
    void operator=(const Type&) = delete;

/// @brief Forbids the allocation on the heap of a given type.
#define DISALLOW_HEAP_ALLOCATION(Type)      \
    void* operator new(size_t)    = delete; \
    void* operator new[](size_t)  = delete; \
    void operator delete(void*)   = delete; \
    void operator delete[](void*) = delete;

/// @brief Convenience macro to define shared pointer types for a given type.
#define DEFINE_SHARED_POINTERS(Tag, Type)         \
    Tag Type;                                     \
    using Type##Ptr      = std::shared_ptr<Type>; \
    using Type##ConstPtr = std::shared_ptr<const Type>

/// @brief Convenience macro to define unique pointer types for a given type.
#define DEFINE_UNIQUE_POINTERS(Tag, Type)         \
    Tag Type;                                     \
    using Type##Ptr      = std::unique_ptr<Type>; \
    using Type##ConstPtr = std::unique_ptr<const Type>

//====================================================================================================================//

/** Trait containing the type that has a higher numeric limits. */
template<typename LEFT, typename RIGHT>
struct higher_type {
    using type = typename std::conditional<std::numeric_limits<LEFT>::max() <= std::numeric_limits<RIGHT>::max(), LEFT,
                                           RIGHT>::type;
};

/// @brief Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value> {
};

//====================================================================================================================//

/** Definitions for the various versions of C++. */

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

namespace std {

/// @brief Void type.
#ifndef NOTF_CPP17
template<class...>
using void_t = void;
#endif

} // namespace std

//====================================================================================================================//

/// @brief Takes two macros and concatenates them without whitespace in between
#define MACRO_CONCAT_(A, B) A##B
#define MACRO_CONCAT(A, B) MACRO_CONCAT_(A, B)

//====================================================================================================================//

/// @brief Short names for unsiged in
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;
