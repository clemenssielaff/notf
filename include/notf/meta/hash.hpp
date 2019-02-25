#pragma once

#include <functional>

#include "notf/meta/pointer.hpp"
#include "notf/meta/real.hpp"
#include "notf/meta/system.hpp"

NOTF_OPEN_NAMESPACE

// hash functions =================================================================================================== //

namespace detail {

/// Changing this value will cause new hashes of the same value (calculated with notf::hash) to differ.
/// This way, we can differentiate between hashes of the same value that were generated with different versions of notf.
constexpr inline size_t versioned_base_hash() noexcept { return config::version_major(); }

/// see http://stackoverflow.com/a/4948967
template<class T>
constexpr size_t magic_hash_number() noexcept {
    long double result = 2.l;
    for (size_t i = 1; i < bitsizeof<T>(); ++i) {
        result *= 2.l;
    }
    return static_cast<size_t>(result / phi());
}

/// Unique identifiers for everything that wants to distinguish its hash value from the value of a similar data type.
/// For example: `auto x = hash(0, 0, 0, 1)` `x` is the same whether the 4 values come from an RGBA color, a Vector4 or
/// any other data type with 4 numbers.
/// By adding the type's HashId into their hash function, we can ensure that two differnent types with the same values
/// will produce different hash values.
enum class HashID : size_t {
    VECTOR2,
    VECTOR3,
    VECTOR4,
    MATRIX3,
    MATRIX4,
    SIZE2,
    AABR,
    RATIONAL,
    COLOR,
    BEZIER,
    CIRCLE,
    TRIANGLE,
    SEGMENT,
    POLYLINE,
    PATH2,
};

} // namespace detail

/// 32 bit mixer taken from MurmurHash3
///     https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
///
/// Use this to improve a hash function with a low entropy (like a counter).
constexpr inline size_t hash_mix(uint key) noexcept {
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
constexpr inline size_t hash_mix(size_t key) noexcept {
    key ^= (key >> 30);
    key *= 0xbf58476d1ce4e5b9;
    key ^= (key >> 27);
    key *= 0x94d049bb133111eb;
    key ^= (key >> 31);
    return key;
}

/// Builds a hash value from hashing all passed data types in sequence and combining their hashes.
/// Similar to boost::hash_combine but adaptive to the system's hash value type.
constexpr inline void hash_combine(std::size_t&) noexcept {}

template<typename Arg, typename... Rest, class T = std::decay_t<Arg>>
constexpr void hash_combine(std::size_t& seed, Arg&& value, Rest&&... rest) noexcept {
    constexpr size_t magic_number = detail::magic_hash_number<size_t>();
    if constexpr (std::is_integral_v<T>) {
        // integral values are mapped on themselves at compile time, saving us a potential non-constexpr call to hash()
        seed ^= static_cast<size_t>(value) + magic_number + (seed << 6) + (seed >> 2);
    } else {
        seed ^= std::hash<T>()(std::forward<Arg>(value)) + magic_number + (seed << 6) + (seed >> 2);
    }
    hash_combine(seed, std::forward<Rest>(rest)...);
}

/// Calculates the combined hash of 0-n supplied values.
/// All passed values must be hashable using std::hash.
template<typename... Values>
constexpr size_t hash(Values&&... values) noexcept {
    std::size_t result = detail::versioned_base_hash();
    hash_combine(result, std::forward<Values>(values)...);
    return result;
}

/// @{
/// Compile time hashing of a const char* array.
/// @param string   String to hash, must be null-terminated.
/// @param size     Size of the string (or at least, how many characters should be hashed).
constexpr size_t hash_string(const char* string, const size_t size) noexcept {
    size_t result = config::constexpr_seed();
    for (size_t i = 0; i < size; ++i) {
        // batch the characters up into a size_t value, so we can use hash_mix on it
        size_t batch = 0;
        for (size_t j = 0; i < size && j < (sizeof(size_t) / sizeof(char)); ++j, ++i) {
            batch |= static_cast<uchar>(string[i]);
            batch <<= bitsizeof<char>();
        }
        hash_combine(result, hash_mix(batch));
    }
    return result;
}
inline size_t hash_string(const std::string& string) noexcept { return hash_string(string.c_str(), string.size()); }
/// @}

/// Specialized Hash for pointers.
/// Uses `hash_mix` to improve pointer entropy.
template<typename T>
struct pointer_hash {
    size_t operator()(const T& ptr) const noexcept { return hash_mix(to_number(raw_pointer(ptr))); }
};

// hashable concept================================================================================================== //

/// @{
/// Constexpr boolean that is true only if T can be hashed using std::hash.
template<class T, class = void>
struct is_hashable : std::false_type {};
template<class T>
struct is_hashable<T, std::void_t<decltype(std::hash<T>()(declval<T>()))>> : std::true_type {};
template<class T>
static constexpr bool is_hashable_v = is_hashable<T>::value;
/// @}

NOTF_CLOSE_NAMESPACE
