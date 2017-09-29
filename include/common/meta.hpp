#pragma once

#include <limits>
#include <type_traits>

/* Extension for variadic compile-time checks.
 * Originally from http://stackoverflow.com/a/17200820/3444217
 */

template <typename T, typename...>
struct is_same_any : std::true_type {
};

template <typename T, typename Head, typename... Rest>
struct is_same_any<T, Head, Rest...> : std::integral_constant<bool, std::is_same<T, Head>::value || is_same_any<T, Rest...>::value> {
};

template <typename T, typename...>
struct is_base_of_any : std::true_type {
};

template <typename T, typename Head, typename... Rest>
struct is_base_of_any<T, Head, Rest...> : std::integral_constant<bool, std::is_base_of<T, Head>::value || is_base_of_any<T, Rest...>::value> {
};

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

#define ENABLE_IF_REAL(TYPE) typename std::enable_if<std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_REAL(TYPE) typename std::enable_if<std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_REAL(TYPE) typename std::enable_if<!std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_REAL(TYPE) typename std::enable_if<!std::is_floating_point<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_INT(TYPE) typename std::enable_if<std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_INT(TYPE) typename std::enable_if<std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_INT(TYPE) typename std::enable_if<!std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_INT(TYPE) typename std::enable_if<!std::is_integral<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SAME(TYPE_A, TYPE_B) typename std::enable_if<std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_SAME(TYPE_A, TYPE_B) typename std::enable_if<std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type

#define ENABLE_IF_SAME_ANY(TYPE_A, ...) typename std::enable_if<is_same_any<typename std::decay<TYPE_A>::type, __VA_ARGS__>::value, bool>::type = true
#define FWD_ENABLE_IF_SAME_ANY(TYPE_A, ...) typename std::enable_if<is_same_any<typename std::decay<TYPE_A>::type, __VA_ARGS__>::value, bool>::type

#define ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B) typename std::enable_if<!std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B) typename std::enable_if<!std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type

#define ENABLE_IF_ARITHMETIC(TYPE) typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_ARITHMETIC(TYPE) typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type

#define DISABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<!std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_DISABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<!std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SUBCLASS_ANY(TYPE, ...) typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS_ANY(TYPE, ...) typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type

//*********************************************************************************************************************/

/** The `always_false` struct can be used to create a templated struct that will always evaluate to `false` when used
 * in a static_assert.
 *
 * Imagine the following scenario: You have a scene graph and a fairly generic operation `calc` on some object class,
 * that you want to be performed in various spaces:
 *
 *     enum class Space {
 *        WORLD,
 *        LOCAL,
 *        SCREEN,
 *     };
 *
 * Instead of passing a `Space` parameter at runtime, or cluttering the interface with `calc_in_world` and
 * `calc_in_local` etc. -operations, you want a template method: `calc<Space>()`.
 * Then you can specialize the template method for various spaces.
 * If a space is *not* supported, you can just not specialize the method for it.
 * The original template method would look this this:
 *
 *     template<Space space>
 *     float calc() {
 *         static_assert(always_false<Space, space>{}, "Unsupported Space");
 *     }
 *
 * And specializations like this:
 *
 *     template<>
 *     float calc<Space:WORLD>() { return 0; }
 *
 * You can now call supported specializations, but unsupported ones will fail at runtime:
 *
 *     calc<Space::WORLD>();  // produces 0
 *     calc<Space::SCREEN>(); // error! static assert: "Unsupported Space"
 *
 */
template <typename T, T val>
struct always_false : std::false_type {
};

/** Like always_false, but taking a single type only.
 * This way you can define error overloads that differ by type only:
 *
 *    static void convert(const Vector4d& in, std::array<float, 4>& out)
 *    {
 *        ...
 *    }
 *
 *    template <typename UNSUPPORTED_TYPE>
 *    static void convert(const Vector4d& in, UNSUPPORTED_TYPE& out)
 *    {
 *        static_assert(always_false_t<T>{}, "Cannot convert a Vector4d into template type T");
 *    }
 *
 */
template <typename T>
struct always_false_t : std::false_type {
};

//*********************************************************************************************************************/

/** Convenience macro to disable the construction of automatic copy- and assign methods.
 * Also disables automatic move constructor/assignment methods, although you might define them yourself, if you want to.
 */
#define DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&) = delete;        \
    void operator=(const Type&) = delete;

/** Convenience macro to define shared pointer types for a given type. */
#define DEFINE_SHARED_POINTERS(Tag, Type)         \
    Tag Type;                                     \
    using Type##Ptr      = std::shared_ptr<Type>; \
    using Type##ConstPtr = std::shared_ptr<const Type>;

//*********************************************************************************************************************/

/** Trait containing the type that has a higher numeric limits. */
template <typename LEFT, typename RIGHT>
struct larger_type {
    using type = std::conditional_t<std::numeric_limits<LEFT>::max() <= std::numeric_limits<RIGHT>::max(),
                                    LEFT,
                                    RIGHT>;
};

//*********************************************************************************************************************/

/** Definitions for the various versions of C++. */

#ifndef __cplusplus
#error A C++ compiler is required!
#else

#if __cplusplus >= 199711L
#define CPP_97
#endif
#if __cplusplus >= 201103L
#define CPP_11
#endif
#if __cplusplus >= 201402L
#define CPP_14
#endif
#if __cplusplus >= 201703L
#define CPP_17
#endif

#endif
