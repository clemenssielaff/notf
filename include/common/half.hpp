#pragma once

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace notf {

/// \brief 16bit floating point type.
/// If I ever need a full-fledged half type, see:
///     https://sourceforge.net/p/half/code/HEAD/tree/tags/release-1.12.0/include/half.hpp
struct half {

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Half value.
    short value;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    half() = default;

    /// @brief Value constructor.
    /// @param value    Float value to convert into a half.
    half(const float value);

    /// @brief Converts the half back to a float.
    explicit operator float() const;
};

inline unsigned int packHalfs(half a, half b)
{
    union {
        half in[2];
        unsigned int out;
    } converter;

    converter.in[0] = a;
    converter.in[1] = b;

    return converter.out;
}

inline std::pair<half, half> unpackHalfs(unsigned int pack)
{
    union {
        unsigned int in;
        half out[2];
    } converter;

    converter.in = pack;

    return std::make_pair(converter.out[0], converter.out[1]);
}

//====================================================================================================================//

/// @brief Prints the value of a half into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param value    Half value.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const half& value);

} // namespace notf
