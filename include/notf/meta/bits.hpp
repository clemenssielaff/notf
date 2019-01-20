#pragma once

#include <utility>

#include "notf/meta/config.hpp"
#include "notf/meta/numeric.hpp"

NOTF_OPEN_NAMESPACE

// bit fiddling ===================================================================================================== //

/// Checks if the n'th bit of number is set (=1).
/// @param number   Number to check
/// @param pos      Position to check (starts at zero).
template<class T>
constexpr bool check_bit(T number, const size_t pos) noexcept {
    return (number >> pos) & 1;
}

/// Checks a given byte against a mask with no significant zeros.
/// This means, that the mask only contains 1 and X, where X denotes an arbitrary value.
/// @param byte     Byte to check.
/// @param mask     Mask to check the byte against.
inline constexpr bool check_byte(const uchar byte, const uchar mask) { return (byte & mask) == mask; }

/// Checks a given byte against a mask with significant zeros.
/// For example, if you want to check if a byte matches the pattern [1001XXXX] (where X denotes an arbitrary value),
/// the mask would be [10010000] and the inverse [01100000].
/// @param byte     Byte to check.
/// @param mask     Positive mask to check the byte against.
/// @param inverse  Negative mask to check the byte against.
inline constexpr bool check_byte(const uchar byte, const uchar mask, const uchar inverse) {
    return ((byte & mask) == mask) && ((~byte & inverse) == inverse);
}

/// Returns the number with ast most the `count` least bits set.
/// @param number   Number to modify.
/// @param count    Number of least significant bits to keep.
template<class T, class Out = std::decay_t<T>>
constexpr Out lowest_bits(T number, const uint count) {
    return number & (exp<T>(2, count) - 1);
}

/// Returns an integral value with all bits set to one.
template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T all_bits_one() {
    return ~T(0);
}

// bit casting ====================================================================================================== //

/// Like `bit_cast` but without any safe guards.
/// Use this only if you know what you are doing, otherwise better use `bit_cast`.
template<typename Dest, typename Source>
inline Dest bit_cast_unsafe(const Source& source) {
    Dest target;
    std::memcpy(&target, &source, sizeof(target));
    return target;
}

/// Save bit_cast equivalent to `*reinterpret_cast<Dest*>(&source)`.
template<typename Dest, typename Source>
inline Dest bit_cast(const Source& source) {
    static_assert(sizeof(Dest) == sizeof(Source), "bit_cast requires source and destination to be the same size");
    static_assert(std::is_trivially_copyable<Dest>::value, "bit_cast requires the destination type to be copyable");
    static_assert(std::is_trivially_copyable<Source>::value, "bit_cast requires the source type to be copyable");
    return bit_cast_unsafe<Dest>(source);
}

// static tests ===================================================================================================== //

static_assert((exp<uchar>(2, 5) - 1) == 0x1f);
static_assert(-static_cast<char>(lowest_bits(uchar(0xef), 5)) == -15);

NOTF_CLOSE_NAMESPACE
