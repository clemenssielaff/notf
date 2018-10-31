#include "notf/common/vector4.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// std::ostream ===================================================================================================== //

std::ostream& operator<<(std::ostream& out, const V4f& vec)
{
    return out << "V4f(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}
std::ostream& operator<<(std::ostream& out, const V4d& vec)
{
    return out << "V4d(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}
std::ostream& operator<<(std::ostream& out, const V4i& vec)
{
    return out << "V4i(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}
std::ostream& operator<<(std::ostream& out, const V4s& vec)
{
    return out << "V4s(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::V4f>);
static_assert(std::is_pod_v<notf::V4d>);
static_assert(std::is_pod_v<notf::V4i>);
static_assert(std::is_pod_v<notf::V4s>);

static_assert(sizeof(V4f) == sizeof(float) * 4);
static_assert(sizeof(V4d) == sizeof(double) * 4);
static_assert(sizeof(V4i) == sizeof(int) * 4);
static_assert(sizeof(V4s) == sizeof(short) * 4);

NOTF_CLOSE_NAMESPACE
