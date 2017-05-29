#pragma once

#include <inttypes.h>
#include <iostream>

#include "tinyutf8/tinyutf8.h"

namespace notf {

using namespace tinyutf8;

/** Native endian UTF-32 as produced by GLFW's `glfwSetCharModsCallback`. */
using utf32_t = uint32_t;

struct Codepoint {
    /** Value contructor. */
    Codepoint(utf32_t value)
        : value(value) {}

    const utf32_t value;
};

/** Prints a single codepoint into a std::ostream.
 * From http://stackoverflow.com/a/23467513/3444217
 * @param out       Output stream.
 * @param codepoint Codepoint to print.
 */
std::ostream& operator<<(std::ostream& out, const Codepoint codepoint);

} // namespace notf
