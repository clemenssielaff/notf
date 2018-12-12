#pragma once

#include "notf/meta/id.hpp"

#include "notf/common/fwd.hpp"

#include "notf/graphic/opengl.hpp"

// forwards ========================================================================================================= //

NOTF_OPEN_NAMESPACE

NOTF_DECLARE_SHARED_POINTERS(class, Font);
NOTF_DECLARE_SHARED_POINTERS(class, FragmentShader);
NOTF_DECLARE_SHARED_POINTERS(class, FrameBuffer);
NOTF_DECLARE_SHARED_POINTERS(class, GeometryShader);
NOTF_DECLARE_SHARED_POINTERS(class, ShaderProgram);
NOTF_DECLARE_SHARED_POINTERS(class, RenderBuffer);
NOTF_DECLARE_SHARED_POINTERS(class, Shader);
NOTF_DECLARE_SHARED_POINTERS(class, TesselationShader);
NOTF_DECLARE_SHARED_POINTERS(class, Texture);
NOTF_DECLARE_SHARED_POINTERS(class, VertexShader);

NOTF_DECLARE_UNIQUE_POINTERS(class, GraphicsContext);
NOTF_DECLARE_UNIQUE_POINTERS(class, VertexArrayType);
NOTF_DECLARE_UNIQUE_POINTERS(class, IndexArrayType);
NOTF_DECLARE_UNIQUE_POINTERS(class, FontManager);

NOTF_DECLARE_SHARED_POINTERS(class, FragmentRenderer);
NOTF_DECLARE_SHARED_POINTERS(class, Plotter);

template<typename>
class PrefabType;

template<typename>
class PrefabFactory;

namespace detail {
class GraphicsSystem;
} // namespace detail
class TheGraphicsSystem;

using RenderBufferId = IdType<RenderBuffer, GLuint>;
using FrameBufferId = IdType<FrameBuffer, GLuint>;
using ShaderId = IdType<Shader, GLuint>;
using TextureId = IdType<Texture, GLuint>;
using ShaderProgramId = IdType<ShaderProgram, GLuint>;

NOTF_CLOSE_NAMESPACE

struct GLFWwindow;
