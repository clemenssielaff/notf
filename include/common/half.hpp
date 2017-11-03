#pragma once

#include <iosfwd>
#include <vector>

namespace notf {

/// \brief 16bit floating point type.
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

//====================================================================================================================//

/// @brief Prints the value of a half into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param value    Half value.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const half& value);

} // namespace notf
