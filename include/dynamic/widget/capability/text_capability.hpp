#pragma once

#include "common/vector2.hpp"
#include "core/capability.hpp"
#include "graphics/text/font.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(struct, TextCapability);

/** Capability used by widgets that contain text that can be placed into the context of a larger, continuous text. */
struct TextCapability : public Capability {

    /** Baseline position at the start of the widget's text relative to the text's top-left corner. */
    Vector2f baseline_start;

    /** Baseline position at the end of the widget's text relative to the text's bottom-right corner. */
    Vector2f baseline_end;

    /** The font used by the widget to display the text. */
    FontPtr font;
};

} // namespace notf
