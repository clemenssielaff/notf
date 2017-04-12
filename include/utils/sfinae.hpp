#pragma once

#include <type_traits>

/* Ath the beginning, the macros here used std::enable_if_t, but typedefs from those templates could not be properly
 * forward defined.
 * Therefore I came up with this method which is similar, also throws a compiler error if the check fails, but has a
 * slightly misleading error message (in gcc: "no type name 'type' in struct std::enable_if<false, bool>).
 *
 * Forward declare a template specialization like this:
 *
 *     template <typename Value_t, FWD_ENABLE_IF_ARITHMETIC(Value_t)>
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

#define ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B) typename std::enable_if<!std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_DIFFERENT(TYPE_A, TYPE_B) typename std::enable_if<!std::is_same<typename std::decay<TYPE_A>::type, typename std::decay<TYPE_B>::type>::value, bool>::type

#define ENABLE_IF_ARITHMETIC(TYPE) typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_ARITHMETIC(TYPE) typename std::enable_if<std::is_arithmetic<typename std::decay<TYPE>::type>::value, bool>::type

#define ENABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type = true
#define FWD_ENABLE_IF_SUBCLASS(TYPE, PARENT) typename std::enable_if<std::is_base_of<PARENT, typename std::decay<TYPE>::type>::value, bool>::type
