#pragma once

#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

struct Scissor {
    /** Scissors have their own transformation. */
    Xform2f xform;

    /** Extend around the center of the Transform. */
    Size2f extend;
};

} // namespace notf
