#pragma once

#include <string_view>

#include "notf/meta/hash.hpp"
#include "./common.hpp"

NOTF_OPEN_NAMESPACE

// string_view ====================================================================================================== //

/// Hashing of a string_view, that is equivalent to the normal compile time hasing of a StringConst or -Type.
/// @param string   String view to hash.
inline size_t hash_string(std::string_view string) noexcept { return hash_string(string.data(), string.size()); }

NOTF_CLOSE_NAMESPACE
