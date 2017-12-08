#pragma once

#include <functional>

#include "common/float.hpp"
#include "common/system.hpp"

namespace notf {

namespace detail {

/// @brief Changing this value will cause new hashes of the same value (calculated with notf::hash) to differ.
/// This way, we can differentiate between hashes of the same value that were calculated with different versions of
/// notf.
constexpr size_t version_hash() { return 0; }

/// @brief Additional value for different semantic types to hash with.
/// Otherwise a Vector4f and a Color value with the same components would produce the same hash.
enum class HashID : size_t {
    VECTOR,
    MATRIX,
    AABR,
    PADDING,
    SIZE,
    COLOR,
    SEGMENT,
    BEZIER,
    CIRCLE,
    TRIANGLE,
    POLYGON,
    // ALWAYS APPEND AT THE END - IF YOU CHANGE EXISTING ENUM VALUES, STORED HASHES WILL NO LONGER MATCH
};

} // namespace detail

//====================================================================================================================//

/// @brief Buils a hash value from hashing all passed data types in sequence and combining their hashes.
/// Similar to boost::hash_combine but adaptive to the system's hash value type.
inline void hash_combine(std::size_t&) {}

template<typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest&&... rest)
{
    // see http://stackoverflow.com/a/4948967 for an explanation of the magic number
    static const size_t magic_number = static_cast<size_t>(powl(2.l, bitsizeof<size_t>()) / ((sqrtl(5.l) + 1) / 2));
    seed ^= std::hash<T>{}(v) + magic_number + (seed << 6) + (seed >> 2);
    hash_combine(seed, std::forward<Rest>(rest)...);
}

/// @brief Calculates the combined hash of 0-n supplied values.
/// All passed values must be hashable using std::hash.
template<typename... Values>
inline size_t hash(Values&&... values)
{
    std::size_t result = detail::version_hash();
    hash_combine(result, std::forward<Values>(values)...);
    return result;
}

} // namespace notf
