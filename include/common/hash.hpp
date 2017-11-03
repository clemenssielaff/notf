#pragma once

#include <functional>

#include "common/float.hpp"
#include "common/system.hpp"

namespace notf {

/** No-op specialization to end the recursion. */
inline void hash_combine(std::size_t&) {}

/** Buils a hash value from hashing all passed data types in sequence and combining their hashes.
 * Similar to boost::hash_combine but adaptive to the system's hash value type.
 */
template<typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest&&... rest)
{
    // see http://stackoverflow.com/a/4948967 for an explanation of the magic number
    static const size_t magic_number = static_cast<size_t>(powl(2.l, bitsizeof<size_t>()) / ((sqrtl(5.l) + 1) / 2));
    seed ^= std::hash<T>{}(v) + magic_number + (seed << 6) + (seed >> 2);
    hash_combine(seed, std::forward<Rest>(rest)...);
}

/** Calculates the combined hash of 0-n supplied values.
 * All passed values must be hashable using std::hash.
 */
template<typename... Values>
inline size_t hash(Values&&... values)
{
    std::size_t result = 0;
    hash_combine(result, std::forward<Values>(values)...);
    return result;
}

} // namespace notf
