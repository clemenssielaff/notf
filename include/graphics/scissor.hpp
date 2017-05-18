#pragma once

#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

/** Scissor.
 * The default constructed Scissor is invalid.
 */
struct Scissor {
    /** Scissors transformation of its top-left corner. */
    Xform2f xform = Xform2f::zero();

    /** Extend of the Scissor in coordinates transformed with `xform`. */
    Size2f extend = Size2f::invalid();
};

} // namespace notf
