#include "notf/common/vector2.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// std::ostream ===================================================================================================== //

std::ostream& operator<<(std::ostream& out, const V2f& vec)
{
    return out << "V2f(" << vec.x() << ", " << vec.y() << ")";
}
std::ostream& operator<<(std::ostream& out, const V2d& vec)
{
    return out << "V2d(" << vec.x() << ", " << vec.y() << ")";
}
std::ostream& operator<<(std::ostream& out, const V2i& vec)
{
    return out << "V2i(" << vec.x() << ", " << vec.y() << ")";
}
std::ostream& operator<<(std::ostream& out, const V2s& vec)
{
    return out << "V2s(" << vec.x() << ", " << vec.y() << ")";
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::V2f>);
static_assert(std::is_pod_v<notf::V2d>);
static_assert(std::is_pod_v<notf::V2i>);
static_assert(std::is_pod_v<notf::V2s>);

static_assert(sizeof(V2f) == sizeof(float) * 2);
static_assert(sizeof(V2d) == sizeof(double) * 2);
static_assert(sizeof(V2i) == sizeof(int) * 2);
static_assert(sizeof(V2s) == sizeof(short) * 2);

NOTF_CLOSE_NAMESPACE
