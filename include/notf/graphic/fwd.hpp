#pragma once

#include "notf/meta/id.hpp"

#include "notf/common/fwd.hpp"

#include "notf/graphic/opengl.hpp"

// forwards ========================================================================================================= //

NOTF_OPEN_NAMESPACE

// NOTF_DECLARE_SHARED_POINTERS(class, FragmentRenderer);

// template<typename>
// class PrefabType;

// template<typename>
// class PrefabFactory;

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

// index_buffer.hpp -------------------------------------------------------- //

//NOTF_DECLARE_SHARED_POINTERS(class, AnyIndexBuffer);
//NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, IndexBuffer);

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

// uniform_buffer.hpp ------------------------------------------------------ //

//NOTF_DECLARE_SHARED_POINTERS(class, AnyUniformBuffer);
//NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, UniformBuffer);

// vertex_buffer.hpp ------------------------------------------------------- //

//NOTF_DECLARE_SHARED_POINTERS_VAR_TEMPLATE(class, VertexBuffer);

// renderer/plotter.hpp ---------------------------------------------------- //

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

using RenderBufferId = IdType<RenderBuffer, GLuint>;
using FrameBufferId = IdType<FrameBuffer, GLuint>;
using ShaderId = IdType<AnyShader, GLuint>;
using TextureId = IdType<Texture, GLuint>;
using ShaderProgramId = IdType<ShaderProgram, GLuint>;

NOTF_CLOSE_NAMESPACE
