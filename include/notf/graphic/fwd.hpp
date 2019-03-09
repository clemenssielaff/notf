#pragma once

#include "notf/meta/id.hpp"
#include "notf/meta/types.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

// opengl buffer types ============================================================================================== //

namespace detail {

/// OpenGL buffer type.
enum class OpenGLBufferType : uchar {
    VERTEX,
    INDEX,
    UNIFORM,
    DRAWCALL,
};

} // namespace detail

// forwards ========================================================================================================= //

// frame_buffer.hpp -------------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, FrameBuffer);
NOTF_DECLARE_SHARED_POINTERS(class, RenderBuffer);

// graphics_context.hpp ---------------------------------------------------- //

NOTF_DECLARE_UNIQUE_POINTERS(class, GraphicsContext);

// graphics_system.hpp ----------------------------------------------------- //

namespace detail {
class GraphicsSystem;
} // namespace detail
class TheGraphicsSystem;

// opengl_buffer.hpp ------------------------------------------------------- //

namespace detail {
template<detail::OpenGLBufferType>
class TypedOpenGLBuffer;
} // namespace detail

// drawcall_buffer.hpp ----------------------------------------------------- //

using DrawCallBuffer = detail::TypedOpenGLBuffer<detail::OpenGLBufferType::DRAWCALL>;
NOTF_DECLARE_SHARED_POINTERS_ONLY(DrawCallBuffer);

// index_buffer.hpp -------------------------------------------------------- //

using AnyIndexBuffer = detail::TypedOpenGLBuffer<detail::OpenGLBufferType::INDEX>;
NOTF_DECLARE_SHARED_POINTERS_ONLY(AnyIndexBuffer);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, IndexBuffer);

// shader.hpp -------------------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, AnyShader);
NOTF_DECLARE_SHARED_POINTERS(class, TesselationShader);
NOTF_DECLARE_SHARED_POINTERS(class, FragmentShader);
NOTF_DECLARE_SHARED_POINTERS(class, GeometryShader);
NOTF_DECLARE_SHARED_POINTERS(class, VertexShader);

// shader_program.hpp ------------------------------------------------------ //

NOTF_DECLARE_SHARED_POINTERS(class, ShaderProgram);

// texture.hpp ------------------------------------------------------------- //

namespace detail {
struct ShaderVariable;
class Uniform;
class UniformBlock;
} // namespace detail
NOTF_DECLARE_SHARED_POINTERS(class, Texture);

// vertex_object.hpp ------------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, VertexObject);

// uniform_buffer.hpp ------------------------------------------------------ //

using AnyUniformBuffer = detail::TypedOpenGLBuffer<detail::OpenGLBufferType::UNIFORM>;
NOTF_DECLARE_SHARED_POINTERS_ONLY(AnyUniformBuffer);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, UniformBuffer);

// vertex_buffer.hpp ------------------------------------------------------- //

using AnyVertexBuffer = detail::TypedOpenGLBuffer<detail::OpenGLBufferType::VERTEX>;
NOTF_DECLARE_SHARED_POINTERS_ONLY(AnyVertexBuffer);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE2(class, VertexBuffer);

// plotter/design.hpp ------------------------------------------------------ //

class PlotterDesign;

// plotter/plotter.hpp ----------------------------------------------------- //

class Painter;

// plotter/plotter.hpp ----------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, Plotter);

// text/font.hpp ----------------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, Font);

// text/font_manager.hpp --------------------------------------------------- //

NOTF_DECLARE_UNIQUE_POINTERS(class, FontManager);


NOTF_CLOSE_NAMESPACE

// glfw.hpp ---------------------------------------------------------------- //
struct GLFWwindow;

// ID types ========================================================================================================= //

NOTF_OPEN_NAMESPACE

using RenderBufferId = IdType<RenderBuffer, uint>;
using FrameBufferId = IdType<FrameBuffer, uint>;
using ShaderId = IdType<AnyShader, uint>;
using TextureId = IdType<Texture, uint>;
using ShaderProgramId = IdType<ShaderProgram, uint>;
using VertexBufferId = IdType<AnyVertexBuffer, uint>;
using IndexBufferId = IdType<AnyIndexBuffer, uint>;
using UniformBufferId = IdType<AnyUniformBuffer, uint>;
using VertexObjectId = IdType<VertexObject, uint>;

NOTF_CLOSE_NAMESPACE
