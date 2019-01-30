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

NOTF_DECLARE_SHARED_POINTERS(class, AnyIndexBuffer);
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

// uniform_buffer.hpp ------------------------------------------------------ //

NOTF_DECLARE_SHARED_POINTERS(class, AnyUniformBuffer);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, UniformBuffer);

// vertex_buffer.hpp ------------------------------------------------------- //

NOTF_DECLARE_SHARED_POINTERS(class, AnyVertexBuffer);
NOTF_DECLARE_SHARED_POINTERS_VAR_TEMPLATE1(class, VertexBuffer);

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
using UniformBufferId = IdType<AnyUniformBuffer, GLuint>;
using VertexBufferId = IdType<AnyVertexBuffer, GLuint>;
using IndexBufferId = IdType<AnyIndexBuffer, GLuint>;

NOTF_CLOSE_NAMESPACE


//left to do:
//    -----------
//    texture.cpp
//        should a texture always bind the grahicssystem when you want to change a paramter?
//    not really, it should only do that if no GraphicsContext is current
//        plotter.cpp
//            all broken
//                graphicsystem
//                    no errors, but not overworked
//        fwd
//            cleanup
