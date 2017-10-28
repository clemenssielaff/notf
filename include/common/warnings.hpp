#pragma once

namespace notf {

/** Macro silencing 'unused parameter / argument' warnings and making it clear that the variable will not be used.
 * (although it doesn't stop from from still using it).
 */
#define UNUSED(x) (void)(x);

/** Useful for manually defining padding bytes in a struct that are only visible to the code model. */
#ifndef __NOTF_NO_MANUAL_PADDING
#define PADDING(x) char __notf_manual_padding[x];
#else
#define PADDING(x)
#endif

} // namespace notf
