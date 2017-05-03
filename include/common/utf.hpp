#pragma once

#include <inttypes.h>

namespace notf {

/** Native endian UTF-32 as produced by GLFW's `glfwSetCharModsCallback`. */
using utf32_t = uint32_t;

} // namespace notf
