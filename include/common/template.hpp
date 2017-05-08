#pragma once

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

#define ENABLE_IF_SUBCLASS_ANY(TYPE, ...) typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS_ANY(TYPE, ...) typename std::enable_if<is_base_of_any<typename std::decay<TYPE>::type, __VA_ARGS__>::value, bool>::type

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
