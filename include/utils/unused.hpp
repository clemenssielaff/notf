#pragma once

namespace notf {

/// Macro silencing 'unused parameter / argument' warnings and making it clear that the variable will not be used.
///
/// (although it doesn't stop from from still using it).
#define UNUSED(x) (void)(x);

} // namespace notf
