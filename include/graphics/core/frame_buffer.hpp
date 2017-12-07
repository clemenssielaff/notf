#pragma once

#include <memory>

#include "./gl_forwards.hpp"
#include "common/forwards.hpp"
#include "common/meta.hpp"
#include "common/size2.hpp"
#include "common/variant.hpp"

namespace notf {

//====================================================================================================================//

/// @brief Base class for all RenderBuffers (color, depth and stencil).
class RenderBuffer final {
    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Type of RenderBuffer.
    enum class Type {
        INVALID,       ///< Invalid value - no RenderBuffer can be of this type.
        COLOR,         ///< Color buffer.
        DEPTH,         ///< Depth buffer.
        STENCIL,       ///< Stencil buffer.
        DEPTH_STENCIL, ///< Buffer combining depth and stencil.
    };

    /// @brief Render buffer arguments.
    struct Args {
        /// @brief Buffer type.
        Type type = Type::INVALID;

        /// @brief Size of the render buffer in pixels.
        Size2s size;

        /// @brief Internal value format of a pixel in the buffer.
        GLenum internal_format = 0;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(RenderBuffer)

    /// @brief Default constructor.
    /// @param context              Graphics context owning the render buffer.
    /// @param args                 Render buffer arguments.
    /// @throws std::runtime_error  If the arguments fail to validate.
    RenderBuffer(GraphicsContextPtr& context, const Args& args);

    /// @brief Destructor.
    ~RenderBuffer();

    /// @brief OpenGL ID of the render buffer.
    GLuint id() const { return m_id; }

    /// @brief Buffer type.
    Type type() const { return m_args.type; }

    /// @brief Size of the render buffer in pixels.
    const Size2s& size() const { return m_args.size; }

    /// @brief Internal value format of a pixel in the buffer.
    GLenum internal_format() const { return m_args.internal_format; }

private:
    /// @brief Checks, whether the given format is a valid internal format for a color render buffer.
    /// @throws std::runtime_error  ...if it isn't.
    static void _assert_color_format(const GLenum internal_format);

    /// @brief Checks, whether the given format is a valid internal format for a depth or stencil render buffer.
    /// @throws std::runtime_error  ...if it isn't.
    static void _assert_depth_stencil_format(const GLenum internal_format);

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// @brief OpenGL ID of the render buffer.
    GLuint m_id;

    /// @brief Render Context owning the render buffer.
    GraphicsContext& m_graphics_context;

    /// @brief Arguments passed to this render buffer.
    Args m_args;
};

//====================================================================================================================//

// TODO: read pixels from framebuffer into raw image
class FrameBuffer final {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using ColorTarget = std::variant<RenderBufferPtr, TexturePtr>;
    using DepthTarget = std::variant<RenderBufferPtr, TexturePtr>;

    /// @brief Arguments used to initialize a Texture.
    /// If you want to set both depth- and stencil targets, both have to refer to the same RenderBuffer and that
    /// RenderBuffer needs a format packing both depth and stencil.
    struct Args {

        /// @brief All color targets
        /// A color target consists of a pair of color buffer id / render target.
        std::vector<std::pair<ushort, ColorTarget>> color_targets;

        /// @brief Depth target.
        DepthTarget depth_target;

        /// @brief Stencil target.
        RenderBufferPtr stencil_target;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// @brief Default constructor.
    /// @param context  Graphics context owning the frane buffer.
    /// @param args     Frame buffer arguments.
    /// @throws std::runtime_error  If the arguments fail to validate.
    FrameBuffer(GraphicsContext& context, Args&& args);

    /// @brief Destructor.
    ~FrameBuffer();

    /// @brief The FrameBuffer's id.
    GLuint id() const { return m_id; }

    /// @brief Texture used as color attachment.
    /// @throws std::runtime_error  If there is no texture attached as the color target.
    const TexturePtr& color_texture(const ushort id);

private:
    /// @brief Checks if we can create a valid frame buffer with the given arguments.
    /// @throws std::runtime_error  ...if we dont.
    void _validate_arguments() const;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief OpenGL ID of the frame buffer.
    GLuint m_id;

    /// @brief Render Context owning the frame buffer.
    GraphicsContext& m_graphics_context;

    /// @brief Arguments passed to this frame buffer.
    Args m_args;
};

} // namespace notf
