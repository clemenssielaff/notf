#pragma once

#include "common/meta.hpp"
#include "robin-map/robin_map.h"

NOTF_OPEN_NAMESPACE

using tsl::robin_map;

/// Tests if the given robin_map offers strong exception guarantee as specified by the implementation.
template<typename KEY, typename VALUE>
constexpr bool has_strong_exception_guarantee(const robin_map<KEY, VALUE>&)
{
    return std::is_nothrow_swappable<std::pair<KEY, VALUE>>::value
           && std::is_nothrow_move_constructible<std::pair<KEY, VALUE>>::value;
}

NOTF_CLOSE_NAMESPACE
