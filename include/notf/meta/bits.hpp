#pragma once

#include <utility>

#include "./config.hpp"
#include "./numeric.hpp"

NOTF_OPEN_NAMESPACE

// bit fiddling ===================================================================================================== //

/// Checks if the n'th bit of number is set (=1).
/// @param number   Number to check
/// @param pos      Position to check (starts at zero).
template<class T>
constexpr bool check_bit(T&& number, const size_t pos) noexcept
{
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
inline constexpr bool check_byte(const uchar byte, const uchar mask, const uchar inverse)
{
    return ((byte & mask) == mask) && ((~byte & inverse) == inverse);
}

/// Returns the number with ast most the `count` least bits set.
/// @param number   Number to modify.
/// @param count    Number of least significant bits to keep.
template<class T, class Out = std::decay_t<T>>
constexpr Out lowest_bits(T&& number, const uint count)
{
    return number & (exp<T>(2, count) - 1);
}

// static tests ===================================================================================================== //

static_assert((exp<uchar>(2, 5) - 1) == 0x1f);
static_assert(-static_cast<char>(lowest_bits(uchar(0xef), 5)) == -15);

NOTF_CLOSE_NAMESPACE
