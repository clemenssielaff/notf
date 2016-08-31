#pragma once

/**
 * Enum Hash Map as described in: https://stackoverflow.com/a/24847480/3444217
 *
 * The problem is that (currently) you cannot use a enum class as key of an unordered_map.
 * This is apparently a defect in the C++ standard and will be fixed at some point,
 * but it seems like right now it isn't.
 */

#include <unordered_map>

namespace signal {

struct _EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
};

template <typename Key>
using _HashType = typename std::conditional<std::is_enum<Key>::value, _EnumClassHash, std::hash<Key>>::type;

template <typename Key, typename T>
using EnumMap = std::unordered_map<Key, T, _HashType<Key>>;

} // namespace signal
