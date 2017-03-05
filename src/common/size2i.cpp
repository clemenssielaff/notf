#include <iostream>
#include <type_traits>

#include "common/size2i.hpp"

#include "common/float.hpp"
#include "common/size2f.hpp"

namespace notf {

Size2i Size2i::from_size2f(const Size2f& size2f)
{
    return {
        static_cast<int>(roundf(size2f.width)),
        static_cast<int>(roundf(size2f.height))};
}

std::ostream& operator<<(std::ostream& out, const Size2i& size)
{
    return out << "Size2i(width: " << size.width << ", height: " << size.height << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Size2i) == sizeof(int) * 2,
              "This compiler seems to inject padding bits into the notf::Size2i memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2i>::value,
              "This compiler does not recognize notf::Size2i as a POD.");

} // namespace notf
