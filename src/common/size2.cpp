#include "common/size2.hpp"

#include <iostream>

namespace notf {

/* Size2f *************************************************************************************************************/

template<>
std::ostream& operator<<(std::ostream& out, const Size2f& size) {
	return out << "Size2f(width: " << size.width << ", height: " << size.height << ")";
}

static_assert(sizeof(Size2f) == sizeof(float) * 2,
              "This compiler seems to inject padding bits into the notf::Size2f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2f>::value, "This compiler does not recognize notf::Size2f as a POD.");

/* Size2i *************************************************************************************************************/

template<>
std::ostream& operator<<(std::ostream& out, const Size2i& size) {
	return out << "Size2i(width: " << size.width << ", height: " << size.height << ")";
}

static_assert(sizeof(Size2i) == sizeof(int) * 2,
              "This compiler seems to inject padding bits into the notf::Size2i memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2f>::value, "This compiler does not recognize notf::Size2i as a POD.");

} // namespace notf
