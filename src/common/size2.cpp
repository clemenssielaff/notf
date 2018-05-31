#include "common/size2.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// Size2f =========================================================================================================== //

namespace detail {

std::ostream& operator<<(std::ostream& out, const Size2f& size)
{
    return out << "Size2f(width: " << size.width << ", height: " << size.height << ")";
}

} // namespace detail

static_assert(sizeof(Size2f) == sizeof(float) * 2,
              "This compiler seems to inject padding bits into the notf::Size2f memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2f>::value, "This compiler does not recognize notf::Size2f as a POD.");

// Size2i =========================================================================================================== //

namespace detail {

std::ostream& operator<<(std::ostream& out, const Size2i& size)
{
    return out << "Size2i(width: " << size.width << ", height: " << size.height << ")";
}

} // namespace detail

static_assert(sizeof(Size2i) == sizeof(int) * 2,
              "This compiler seems to inject padding bits into the notf::Size2i memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2f>::value, "This compiler does not recognize notf::Size2i as a POD.");

// Size2s =========================================================================================================== //

namespace detail {

std::ostream& operator<<(std::ostream& out, const Size2s& size)
{
    return out << "Size2s(width: " << size.width << ", height: " << size.height << ")";
}

} // namespace detail

static_assert(sizeof(Size2s) == sizeof(short) * 2,
              "This compiler seems to inject padding bits into the notf::Size2s memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2s>::value, "This compiler does not recognize notf::Size2s as a POD.");

NOTF_CLOSE_NAMESPACE
