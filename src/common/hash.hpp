#pragma once

#include <functional>

#include "common/float.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

/// Changing this value will cause new hashes of the same value (calculated with notf::hash) to differ.
/// This way, we can differentiate between hashes of the same value that were generated with different versions of NoTF.
constexpr size_t version_hash() { return 0; }

/// Additional value for different semantic types to hash with.
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

// ================================================================================================================== //

/// Buils a hash value from hashing all passed data types in sequence and combining their hashes.
/// Similar to boost::hash_combine but adaptive to the system's hash value type.
inline void hash_combine(std::size_t&) {}

template<typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest&&... rest)
{
    // see http://stackoverflow.com/a/4948967 for an explanation of the magic number
    static const size_t magic_number = static_cast<size_t>(powl(2.l, static_cast<double>(bitsizeof<size_t>())) / ((sqrtl(5.l) + 1) / 2));
    seed ^= std::hash<T>{}(v) + magic_number + (seed << 6) + (seed >> 2);
    hash_combine(seed, std::forward<Rest>(rest)...);
}

/// Calculates the combined hash of 0-n supplied values.
/// All passed values must be hashable using std::hash.
template<typename... Values>
inline size_t hash(Values&&... values)
{
    std::size_t result = detail::version_hash();
    hash_combine(result, std::forward<Values>(values)...);
    return result;
}

/// 32 bit mixer taken from MurmurHash3
///     https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
///
/// Use this to improve a hash function with a low entropy (like a counter).
constexpr inline size_t hash_mix(uint key)
{
    key ^= key >> 16;
    key *= 0x85ebca6b;
    key ^= key >> 13;
    key *= 0xc2b2ae35;
    key ^= key >> 16;
    return key;
}

/// 64 bit mixer as described (Mix 13) in
///     https://zimbry.blogspot.co.nz/2011/09/better-bit-mixing-improving-on.html
/// which is based on the mixer used in MurmurHash3
///     https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
///
/// Use this to improve a hash function with a low entropy (like a counter).
constexpr inline size_t hash_mix(size_t key)
{
    key ^= (key >> 30);
    key *= 0xbf58476d1ce4e5b9;
    key ^= (key >> 27);
    key *= 0x94d049bb133111eb;
    key ^= (key >> 31);
    return key;
}

NOTF_CLOSE_NAMESPACE
