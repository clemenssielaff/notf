#pragma once

#include <limits.h>
#include <utility>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<class T>
constexpr std::size_t bitsizeof()
{
    return sizeof(T) * CHAR_BIT;
}

/// Checks this system's endianness.
/// From https://stackoverflow.com/a/4240036
inline constexpr bool is_big_endian() { return 'ABCD' == 0x41424344; }

NOTF_CLOSE_NAMESPACE
