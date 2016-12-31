#pragma once

/**
 * Macro overloading for up to 16 parameters.
 * Use to define "MY_MACRO" as follows:
 *
 *     #define MY_MACRO(...) NOTF_OVERLOADED_MACRO(MY_MACRO, __VA_ARGS__)
 *     #define MY_MACRO1(x) <implementation>
 *     #define MY_MACRO2(x, y) <implementation>
 *     #define MY_MACRO3(x, y, z) <implementation>
 *
 * From http://stackoverflow.com/a/30566098/3444217
 */
#define _notf_overload_expand(macro_name, number_of_args) macro_name##number_of_args
#define _notf_overload(macro_name, number_of_args) _notf_overload_expand(macro_name, number_of_args)
#define _notf_arg_pattern_match(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, n, ...) n
#define _notf_count_args(...) _notf_arg_pattern_match(__VA_ARGS__, 16, 15, 14, 13, 13, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define NOTF_OVERLOADED_MACRO(macro_name, ...) _notf_overload(macro_name, _notf_count_args(__VA_ARGS__))(__VA_ARGS__)
