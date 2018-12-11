#pragma once

#include <memory>

#include "notf/meta/fwd.hpp"
#include "notf/meta/half.hpp"
#include "notf/meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// arithmetic.hpp
namespace detail {
template<class, class, class, size_t>
struct Arithmetic;
} // namespace detail

// color.hpp
struct Color;

// matrix3.hpp
namespace detail {
template<class>
struct Matrix3;
}
using M3f = detail::Matrix3<float>;
using M3d = detail::Matrix3<double>;

// matrix4.hpp
namespace detail {
template<class>
struct Matrix4;
}
using M4f = detail::Matrix4<float>;
using M4d = detail::Matrix4<double>;

// msgpack.hpp
class MsgPack;

// thread.hpp
class Thread;

// uuid.hpp
class Uuid;

// vector2.hpp
namespace detail {
template<class>
struct Vector2;
}
using V2h = detail::Vector2<half>;
using V2f = detail::Vector2<float>;
using V2d = detail::Vector2<double>;
using V2i = detail::Vector2<int>;
using V2s = detail::Vector2<short>;

// vector3.hpp
namespace detail {
template<class>
struct Vector3;
}
using V3h = detail::Vector3<half>;
using V3f = detail::Vector3<float>;
using V3d = detail::Vector3<double>;
using V3i = detail::Vector3<int>;
using V3s = detail::Vector3<short>;

// vector4.hpp
namespace detail {
template<class>
struct Vector4;
}
using V4h = detail::Vector4<half>;
using V4f = detail::Vector4<float>;
using V4d = detail::Vector4<double>;
using V4i = detail::Vector4<int>;
using V4s = detail::Vector4<short>;

NOTF_CLOSE_NAMESPACE
