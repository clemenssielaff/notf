#pragma once

#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

struct Scissor {
    /** Scissors have their own transformation. */
    Xform2f xform = Xform2f::identity();

    /** Extend around the center of the Transform. */
    Size2f extend = Size2f::zero();
};

} // namespace notf
