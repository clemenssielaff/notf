#pragma once

#include "common/meta.hpp"

#include <limits.h>
#include <stddef.h>

NOTF_OPEN_NAMESPACE

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<typename T>
inline constexpr size_t bitsizeof()
{
    return sizeof(T) * CHAR_BIT;
}

NOTF_CLOSE_NAMESPACE
