#pragma once

#include <memory>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

template<typename REAL>
struct RealVector2;

template<typename INTEGER>
struct IntVector2;

template<typename REAL>
struct RealVector3;

template<typename INTEGER>
struct IntVector3;

template<typename REAL>
struct RealVector4;

template<typename REAL>
struct Matrix3;

template<typename REAL>
struct Matrix4;

template<size_t ORDER, typename VECTOR>
struct Bezier;

template<typename REAL>
struct Polygon;

} // namespace detail

struct half;

using Vector2f = detail::RealVector2<float>;
using Vector2d = detail::RealVector2<double>;
using Vector2h = detail::RealVector2<half>;
using Vector2i = detail::IntVector2<int>;
using Vector2s = detail::IntVector2<short>;

using Vector3f = detail::RealVector3<float>;
using Vector3d = detail::RealVector3<double>;
using Vector3h = detail::RealVector3<half>;
using Vector3i = detail::IntVector3<int>;

using Vector4f = detail::RealVector4<float>;
using Vector4d = detail::RealVector4<double>;
using Vector4h = detail::RealVector4<half>;

using Matrix3f = detail::Matrix3<float>;
using Matrix3d = detail::Matrix3<double>;

using Matrix4f = detail::Matrix4<float>;
using Matrix4d = detail::Matrix4<double>;

struct Color;

using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;

using CubicBezier2f = detail::Bezier<3, Vector2f>;

using Polygonf = detail::Polygon<float>;

NOTF_DEFINE_UNIQUE_POINTERS(class, LogHandler);
NOTF_DEFINE_UNIQUE_POINTERS(class, ThreadPool);

NOTF_CLOSE_NAMESPACE