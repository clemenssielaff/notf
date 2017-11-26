#pragma once

#include <memory>

#include "common/meta.hpp"

namespace notf {

// common ============================================================================================================//

template<typename Real>
struct _RealVector2;
using Vector2f = _RealVector2<float>;
using Vector2d = _RealVector2<double>;

template<typename Real>
struct _RealVector4;
using Vector4f = _RealVector4<float>;

template<typename Real>
struct _Matrix4;
using Matrix4f = _Matrix4<float>;

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

} // namespace notf
