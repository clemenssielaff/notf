#pragma once

#include <memory>

#include "notf/meta/fwd.hpp"
#include "notf/meta/half.hpp"
#include "notf/meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// aabr.hpp
namespace detail {
template<class>
struct Aabr;
}
using Aabrf = detail::Aabr<float>;
using Aabrd = detail::Aabr<double>;
using Aabri = detail::Aabr<int>;
using Aabrs = detail::Aabr<short>;

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

// size2.hpp
namespace detail {
template<class>
struct Size2;
}
using Size2f = detail::Size2<float>;
using Size2d = detail::Size2<double>;
using Size2i = detail::Size2<int>;
using Size2s = detail::Size2<short>;

// bezier.hpp
namespace detail {
template<size_t, class>
struct Bezier;
}
using CubicBezier2f = detail::Bezier<3, V2f>;
using CubicBezier2d = detail::Bezier<3, V2d>;

// circle.hpp
namespace detail {
template<class>
struct Circle;
}
using Circlef = detail::Circle<float>;

// polygon.hpp
namespace detail {
template<class>
class Polygon;
}
using Polygonf = detail::Polygon<float>;

// segment.hpp
namespace detail {
template<class>
struct Segment2;
template<class>
struct Segment3;
} // namespace detail
using Segment2f = detail::Segment2<float>;
using Segment3f = detail::Segment3<float>;

// triangle.hpp
namespace detail {
template<class>
struct Triangle;
}
using Trianglef = detail::Triangle<float>;

// free templates =================================================================================================== //

/// Conversion template declaration.
template<class To, class From>
To convert_to(const From& from);

/// Transforms a given value.
template<class T, class Matrix>
T transform_by(const T& value, const Matrix& matrix);

NOTF_CLOSE_NAMESPACE
