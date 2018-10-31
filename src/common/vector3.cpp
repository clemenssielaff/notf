#include "notf/common/vector3.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// std::ostream ===================================================================================================== //

std::ostream& operator<<(std::ostream& out, const V3f& vec)
{
    return out << "V3f(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
}
std::ostream& operator<<(std::ostream& out, const V3d& vec)
{
    return out << "V3d(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
}
std::ostream& operator<<(std::ostream& out, const V3i& vec)
{
    return out << "V3i(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
}
std::ostream& operator<<(std::ostream& out, const V3s& vec)
{
    return out << "V3s(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::V3f>);
static_assert(std::is_pod_v<notf::V3d>);
static_assert(std::is_pod_v<notf::V3i>);
static_assert(std::is_pod_v<notf::V3s>);

static_assert(sizeof(V3f) == sizeof(float) * 3);
static_assert(sizeof(V3d) == sizeof(double) * 3);
static_assert(sizeof(V3i) == sizeof(int) * 3);
static_assert(sizeof(V3s) == sizeof(short) * 3);

NOTF_CLOSE_NAMESPACE
