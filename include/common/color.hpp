#pragma once

#include <iosfwd>

#include "common/real.hpp"

namespace notf {

/// @brief The Color class.
struct Color {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Red component in range [0, 1].
    Real r;

    /// @brief Green component in range [0, 1].
    Real g;

    /// @brief Blue component in range [0, 1].
    Real b;

    /// @brief Alpha component in range [0, 1].
    Real a;
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Prints the contents of this Color into a std::ostream.
///
/// @param os       Output stream, implicitly passed with the << operator.
/// @param color    Color to print.
///
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Color& color);

} // namespace notf
