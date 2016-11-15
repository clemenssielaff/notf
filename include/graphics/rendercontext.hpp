#pragma once

#include "common/size2i.hpp"
#include "common/vector2.hpp"

struct NVGcontext;

namespace notf {

/**********************************************************************************************************************/

struct RenderContext {

    /** NanoVG context to draw into. */
    NVGcontext* nanovg_context;

    /** Size of the Window in screen coordinates (not pixels). */
    Size2i window_size;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2i buffer_size;

    /** Position of the cursor, in screen coordinates, relative to the upper-left corner. */
    Vector2 mouse_pos;

    /** The pixel aspect ratio. */
    float get_pixel_ratio() const { return window_size.width ? static_cast<float>(buffer_size.width) / static_cast<float>(window_size.width)
                                                             : 0.f; }
};

} // namespace notf
