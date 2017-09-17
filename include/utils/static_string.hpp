#pragma once

#include <stddef.h>

namespace notf {

/** constexpr string.
 * Inspired by Scott Schurr's:
 * https://github.com/boostcon/cppnow_presentations_2012/blob/master/wed/schurr_cpp11_tools_for_class_authors.pdf
 */
struct _StaticString {
    const char* const ptr;
    const size_t size;

    /*** Constructor */
    template <size_t N>
    constexpr _StaticString(const char (&string)[N])
        : ptr(string)
        , size(N - 1)
    {
    }
};

#define StaticString constexpr static _StaticString

} // namespace notf
