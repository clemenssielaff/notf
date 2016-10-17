#pragma once

#include <iosfwd>

#include "common/real.hpp"

namespace signal {

/// \brief The Size2r class.
struct Size2r {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Width.
    Real width;

    /// \brief Height.
    Real height;

    //  OPERATORS  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Equal comparison with another Size2r.
    bool operator==(const Size2r& other) const { return approx(other.width, width) && approx(other.height, height); }

    /// \brief Not-equal comparison with another Size2r.
    bool operator!=(const Size2r& other) const { return !approx(other.width, width) || !approx(other.height, height); }
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Prints the contents of this Size2r into a std::ostream.
///
/// \param os   Output stream, implicitly passed with the << operator.
/// \param size Size2 to print.
///
/// \return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Size2r& size);

} // namespace signal
