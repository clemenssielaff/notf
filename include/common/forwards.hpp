#pragma once

#include <memory>

#include "common/meta.hpp"

namespace notf {

// common ============================================================================================================//

namespace detail {

template<typename Real>
struct RealVector2;

template<typename INTEGER>
struct IntVector2;

template<typename Real>
struct RealVector3;

template<typename INTEGER>
struct IntVector3;

template<typename Real>
struct RealVector4;

template<typename Real>
struct Matrix3;

template<typename Real>
struct Matrix4;

} // namespace detail

struct half;

using Vector2f = detail::RealVector2<float>;
using Vector2d = detail::RealVector2<double>;
using Vector2h = detail::RealVector2<half>;
using Vector2i = detail::IntVector2<int>;

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

// graphics ==========================================================================================================//

DEFINE_SHARED_POINTERS(class, Shader);
DEFINE_SHARED_POINTERS(class, VertexShader);
DEFINE_SHARED_POINTERS(class, FragmentShader);
DEFINE_SHARED_POINTERS(class, GeometryShader);
DEFINE_SHARED_POINTERS(class, Texture);
DEFINE_SHARED_POINTERS(class, TesselationShader);
DEFINE_SHARED_POINTERS(class, FrameBuffer);
DEFINE_SHARED_POINTERS(class, RenderBuffer);
DEFINE_SHARED_POINTERS(class, Pipeline);

DEFINE_UNIQUE_POINTERS(class, GraphicsContext);
DEFINE_UNIQUE_POINTERS(class, VertexArrayType);
DEFINE_UNIQUE_POINTERS(class, IndexArrayType);

} // namespace notf
