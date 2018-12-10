#pragma once

#include <bitset>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// @{
/// Returns the size of a bitset at compile time.
template<class T>
struct bitset_size;
template<size_t Size>
struct bitset_size<std::bitset<Size>> {
    static constexpr size_t value = Size;
};
template<class T>
static constexpr size_t bitset_size_v = bitset_size<T>::value;
/// @}

NOTF_CLOSE_NAMESPACE
