#pragma once

#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

/** Scissor.
 * The default constructed Scissor is invalid.
 */
struct Scissor {
    /** Scissors transformation.
     * Note that the transformation of the scissor points to the center of the scissor, not it's top-left corner.
     */
    Xform2f xform = Xform2f::zero();

    /** Extend of the Scissor. */
    Size2f extend = Size2f::invalid();
};

} // namespace notf
