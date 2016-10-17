#pragma once

#include <iosfwd>

namespace signal {

/// \brief The Size2i class.
struct Size2i {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Width.
    unsigned int width;

    /// \brief Height.
    unsigned int height;

    //  OPERATORS  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// \brief Equal comparison with another Size2i.
    bool operator==(const Size2i& other) const { return (other.width == width && other.height == height); }

    /// \brief Not-equal comparison with another Size2i.
    bool operator!=(const Size2i& other) const { return (other.width != width || other.height != height); }
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Prints the contents of this Size2i into a std::ostream.
///
/// \param os   Output stream, implicitly passed with the << operator.
/// \param size Size2 to print.
///
/// \return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Size2i& size);

} // namespace signal
