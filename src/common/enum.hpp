#pragma once

#include <unordered_map>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

struct EnumClassHash {
    template<typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

template<typename Key>
using HashType = typename std::conditional<std::is_enum<Key>::value, EnumClassHash, std::hash<Key>>::type;

constexpr size_t _bit_index_recursion(size_t c, size_t v) { return v == 0 ? c : _bit_index_recursion(c + 1, v >> 1); }

} // namespace detail

/**
 * Enum Hash Map as described in: https://stackoverflow.com/a/24847480
 *
 * The problem is that (currently) you cannot use a enum class as key of an unordered_map.
 * This is apparently a defect in the C++ standard and will be fixed at some point,
 * but it seems like right now it isn't.
 */
template<typename Key, typename T>
using EnumMap = std::unordered_map<Key, T, detail::HashType<Key>>;

/// If you have an enum that acts as container for flags and has power-of-two values, you can use this constant
/// expression to transform the value into an index, for example for a bitset.
///
///      enum Flags : size_t {
///          A = 1u << 0,
///          B = 1u << 1,
///          C = 1u << 2,
///          D = 1u << 3,
///          _LAST,
///      };
///      bit_index(C) == 2; // is true
///      bit_index(D) == 3; // is true
///
constexpr size_t bit_index(size_t v) { return detail::_bit_index_recursion(0, (v ^ (v - 1)) >> 1); }

/// Convenience constexpr for _LAST members, that don't have a power-of-two value.
constexpr size_t bit_index_count(size_t v) { return bit_index(v - 1) + 1; }

NOTF_CLOSE_NAMESPACE
