#include "common/vector3.hpp"

namespace notf {

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Vector3) == sizeof(float) * 3,
              "This compiler seems to inject padding bits into the notf::Vector3 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector3>::value,
              "This compiler does not recognize notf::Vector3 as a POD.");

} // namespace notf
