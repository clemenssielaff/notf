#pragma once

#include <functional>

namespace notf {

/** No-op specialization to end the recursion. */
inline void hash_combine(std::size_t&) {}

/** Buils a hash value from hashing all passed data types in sequence and combining their hashes.
 * From http://stackoverflow.com/a/38140932/3444217
 */
template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest&&... rest)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, std::forward<Rest>(rest)...);
}

/** Calculates the combined hash of 0-n supplied values.
 * All passed values must be hashable using std::hash.
 */
template <typename... Values>
inline size_t hash(Values&&... values)
{
    std::size_t result = 0;
    hash_combine(result, std::forward<Values>(values)...);
    return result;
}

} // namespace notf
