#pragma once

#include <unordered_map>

namespace notf {

namespace detail {

struct _EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
};

template <typename Key>
using _HashType = typename std::conditional<std::is_enum<Key>::value, _EnumClassHash, std::hash<Key>>::type;

constexpr size_t _bit_index_recursion(size_t c, size_t v) { return v == 0 ? c : _bit_index_recursion(c + 1, v >> 1); }

} // namespace detail

/**
 * Enum Hash Map as described in: https://stackoverflow.com/a/24847480/3444217
 *
 * The problem is that (currently) you cannot use a enum class as key of an unordered_map.
 * This is apparently a defect in the C++ standard and will be fixed at some point,
 * but it seems like right now it isn't.
 */
template <typename Key, typename T>
using EnumMap = std::unordered_map<Key, T, detail::_HashType<Key>>;

/**
 * If you have an enum that acts as container for flags and has power-of-two values, you can use this constant
 * expression to transform the value into an index, for example for a bitset.
 * Adapted from: https://g...content-available-to-author-only...d.edu/~seander/bithacks.html#ZerosOnRightLinear
 *
 *      enum Flags : size_t {
 *          A = 1u << 0,
 *          B = 1u << 1,
 *          C = 1u << 2,
 *          D = 1u << 3,
 *          _LAST,
 *      };
 *      bit_index(D) == 3;
 */
constexpr size_t bit_index(size_t v) { return detail::_bit_index_recursion(0, (v ^ (v - 1)) >> 1); }

/** Convenience constexpr for _LAST members (see example for `bit_index`). */
constexpr size_t bit_index_count(size_t v) { return bit_index(v - 1) + 1; }

/** Constexpr to use an enum class value as a numeric value.
 * Blatantly copied from "Effective Modern C++ by Scott Mayers': Item #10.
 */
template <typename Enum>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

} // namespace notf