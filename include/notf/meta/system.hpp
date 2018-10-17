#pragma once

#include <climits>
#include <utility>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<class T>
constexpr std::size_t bitsizeof() noexcept
{
    return sizeof(T) * CHAR_BIT;
}

NOTF_CLOSE_NAMESPACE
