#pragma once

#include <stddef.h>

#include "common/identifier.hpp"

namespace notf {

class Font;

/** To allow other objects to keep references to a Font without handing them volatile pointers or split ownership with
 * shared pointers, everyone can request a `FontID` from the manager.
 * The FontId corresponds to an index in a vector in the FontManager, meaning lookup of the actual Font object given its
 * ID is trivial.
 */
using FontID = Id<Font, size_t>;

} // namespace notf
