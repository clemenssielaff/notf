#pragma once

#include "common/forwards.hpp"
#include "common/id.hpp"
#include "common/meta.hpp"
#include "common/size2.hpp"
#include "common/variant.hpp"
#include "graphics/forwards.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// RenderBuffer ID type.
using RenderBufferId = IdType<RenderBuffer, GLuint>;
static_assert(std::is_pod<RenderBufferId>::value, "RenderBufferId is not a POD type");

/// FrameBuffer ID type.
using FrameBufferId = IdType<FrameBuffer, GLuint>;
static_assert(std::is_pod<FrameBufferId>::value, "FrameBufferId is not a POD type");

//====================================================================================================================//

/// Base class for all RenderBuffers (color, depth and stencil).
/// RenderBuffers are not managed by the GraphicsContext, but by FrameBuffers instead.
/// However, if the underlying GraphicContext should be destroyed, all RenderBuffer objects are deallocated as well.
class RenderBuffer {
    friend class FrameBuffer;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Type of RenderBuffer.
    enum class Type {
        INVALID,       ///< Invalid value - no RenderBuffer can be of this type.
        COLOR,         ///< Color buffer.
        DEPTH,         ///< Depth buffer.
        STENCIL,       ///< Stencil buffer.
        DEPTH_STENCIL, ///< Buffer combining depth and stencil.
    };

    /// Render buffer arguments.
    struct Args {
        /// Buffer type.
        Type type = Type::INVALID;

        /// Size of the render buffer in pixels.
        Size2s size;

        /// Internal value format of a pixel in the buffer.
        GLenum internal_format = 0;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param context         Graphics context owning the render buffer.
    /// @param args            Render buffer arguments.
    /// @throws runtime_error  If the arguments fail to validate.
    RenderBuffer(GraphicsContextPtr& context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderBuffer)

    /// Factory.
    /// @param context         Graphics context owning the render buffer.
    /// @param args            Render buffer arguments.
    /// @throws runtime_error  If the arguments fail to validate.
    static RenderBufferPtr create(GraphicsContextPtr& context, Args&& args);

    /// Destructor.
    ~RenderBuffer();

    /// OpenGL ID of the render buffer.
    RenderBufferId id() const { return m_id; }

    /// Buffer type.
    Type type() const { return m_args.type; }

    /// Size of the render buffer in pixels.
    const Size2s& size() const { return m_args.size; }

    /// Internal value format of a pixel in the buffer.
    GLenum internal_format() const { return m_args.internal_format; }

private:
    /// Checks, whether the given format is a valid internal format for a color render buffer.
    /// @throws runtime_error   ...if it isn't.
    static void _assert_color_format(const GLenum internal_format);

    /// Checks, whether the given format is a valid internal format for a depth or stencil render buffer.
    /// @throws runtime_error  ...if it isn't.
    static void _assert_depth_stencil_format(const GLenum internal_format);

    /// Deallocates the framebuffer data and invalidates the RenderBuffer.
    void _deallocate();

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// OpenGL ID of the render buffer.
    RenderBufferId m_id;

    /// Render Context owning the render buffer.
    GraphicsContext& m_graphics_context;

    /// Arguments passed to this render buffer.
    Args m_args;
};

//====================================================================================================================//

// TODO: read pixels from framebuffer into raw image
class FrameBuffer {
    friend class GraphicsContext;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// A FrameBuffer's color target can be either a RenderBuffer or a Texture.
    using ColorTarget = std::variant<TexturePtr, RenderBufferPtr>;

    /// A FrameBuffer's depth target can be either a RenderBuffer or a Texture.
    using DepthTarget = std::variant<TexturePtr, RenderBufferPtr>;

    /// Arguments used to initialize a Texture.
    /// If you want to set both depth- and stencil targets, both have to refer to the same RenderBuffer and that
    /// RenderBuffer needs a format packing both depth and stencil.
    struct Args {

        /// Defines a color target.
        /// If the id already identifies a color target, it is updated.
        /// @param id       Color target id.
        /// @param target   Color target.
        void set_color_target(const ushort id, ColorTarget target);

        /// All color targets
        /// A color target consists of a pair of color buffer id / render target.
        std::vector<std::pair<ushort, ColorTarget>> color_targets;

        /// Depth target.
        DepthTarget depth_target;

        /// Stencil target.
        RenderBufferPtr stencil_target;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param context          Graphics context owning the frane buffer.
    /// @param args             Frame buffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    FrameBuffer(GraphicsContext& context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(FrameBuffer)

    /// Factory.
    /// @param context          Graphics context owning the frane buffer.
    /// @param args             Frame buffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static FrameBufferPtr create(GraphicsContext& context, Args&& args);

    /// Destructor.
    ~FrameBuffer();

    /// The FrameBuffer's id.
    FrameBufferId id() const { return m_id; }

    /// Texture used as color attachment.
    /// @throws runtime_error   If there is no texture attached as the color target.
    const TexturePtr& color_texture(const ushort id);

private:
    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    void _deallocate();

    /// Checks if we can create a valid frame buffer with the given arguments.
    /// @throws runtime_error   ...if we dont.
    void _validate_arguments() const;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Render Context owning the frame buffer.
    GraphicsContext& m_graphics_context;

    /// OpenGL ID of the frame buffer.
    FrameBufferId m_id;

    /// Arguments passed to this frame buffer.
    Args m_args;
};

NOTF_CLOSE_NAMESPACE