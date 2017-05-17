#pragma once

#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

struct Scissor {
    /** Scissors have their own transformation. */
    Xform2f xform = Xform2f::identity();

    /** Extend of the Scissor. */
    Size2f extend = Size2f::invalid();
};

} // namespace notf
