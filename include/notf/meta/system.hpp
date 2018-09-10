#pragma once

#include <cstdint>
#include <limits.h>

#include "./meta.hpp"

NOTF_OPEN_META_NAMESPACE

// ================================================================================================================== //

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<class T>
inline constexpr std::size_t bitsizeof()
{
    return sizeof(T) * CHAR_BIT;
}

NOTF_CLOSE_META_NAMESPACE
