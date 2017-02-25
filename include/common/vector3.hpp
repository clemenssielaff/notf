#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/float_utils.hpp"

namespace notf {

struct Vector3 { // TODO: real Vector3 class

    float x;

    float y;

    float z;

    /** Direct read-only memory access to the Vector. */
    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    const float& operator[](T&& row) const
    {
        assert(0 <= row && row <= 1);
        return *(&x + row);
    }

    /** Direct read/write memory access to the Vector. */
    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    float& operator[](T&& row)
    {
        assert(0 <= row && row <= 1);
        return *(&x + row);
    }
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Prints the contents of this Vector3 into a std::ostream.
///
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector3 to print.
///
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector3& vec);


} // namespace notf
