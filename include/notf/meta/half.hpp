#pragma once

#include <iosfwd>
#include <utility>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// half ============================================================================================================= //

/// 16bit floating point type.
/// If you ever need a full-fledged half type, see:
///     https://sourceforge.net/p/half/code/HEAD/tree/tags/release-1.12.0/include/half.hpp
struct half {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    half() = default;

    /// Value constructor.
    /// @param value    Float value to convert into a half.
    half(const float value);

    /// Converts the half back to a float.
    explicit operator float() const;

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Half value.
    short value;
};

/// Pack two halfs into a 32-bit unsigned integer.
/// @param a    First half.
/// @param b    Second half.
/// @returns    Unsigned int containing two halfs.
inline unsigned int packHalfs(half a, half b) noexcept {
    union {
        half in[2];
        unsigned int out;
    } converter;

    converter.in[0] = a;
    converter.in[1] = b;

    return converter.out;
}

/// Unpacks two halfs from a 32-bit unsigned integer.
/// @param pack Unsigned int to unpack.
/// @returns    Pair of halfs.
inline std::pair<half, half> unpackHalfs(unsigned int pack) noexcept {
    union {
        unsigned int in;
        half out[2];
    } converter;

    converter.in = pack;

    return std::make_pair(converter.out[0], converter.out[1]);
}

// formatting ======================================================================================================= //

/// Prints the value of a half into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param value    Half value.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const half& value);

NOTF_CLOSE_NAMESPACE
