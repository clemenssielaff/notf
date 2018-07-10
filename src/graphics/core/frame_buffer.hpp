#pragma once

#include <vector>

#include "common/size2.hpp"
#include "common/variant.hpp"
#include "graphics/ids.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _RenderBuffer;
template<class>
class _FrameBuffer;
} // namespace access

// ================================================================================================================== //

/// Base class for all RenderBuffers (color, depth and stencil).
/// RenderBuffers are not managed by the GraphicsContext, but by FrameBuffers instead.
/// However, if the underlying GraphicContext should be destroyed, all RenderBuffer objects are deallocated as well.
class RenderBuffer {

    friend class access::_RenderBuffer<FrameBuffer>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_RenderBuffer<T>;

    /// Type of RenderBuffer.
    enum class Type {
        COLOR,         ///< Color buffer.
        DEPTH,         ///< Depth buffer.
        STENCIL,       ///< Stencil buffer.
        DEPTH_STENCIL, ///< Buffer combining depth and stencil.
    };

    /// Render buffer arguments.
    struct Args {
        /// Buffer type.
        Type type = Type::COLOR;

        /// Size of the render buffer in pixels.
        Size2s size;

        /// Internal value format of a pixel in the buffer.
        GLenum internal_format = 0;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param context         Graphics context owning the render buffer.
    /// @param args            Render buffer arguments.
    /// @throws runtime_error  If the arguments fail to validate.
    RenderBuffer(GraphicsContextPtr& context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderBuffer);

    /// Factory.
    /// @param context         Graphics context owning the render buffer.
    /// @param args            Render buffer arguments.
    /// @throws runtime_error  If the arguments fail to validate.
    static RenderBufferPtr create(GraphicsContextPtr& context, Args&& args)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(RenderBuffer, context, std::move(args));
    }

    /// Destructor.
    ~RenderBuffer();

    /// OpenGL ID of the render buffer.
    RenderBufferId get_id() const { return m_id; }

    /// Buffer type.
    Type get_type() const { return m_args.type; }

    /// Size of the render buffer in pixels.
    const Size2s& get_size() const { return m_args.size; }

    /// Internal value format of a pixel in the buffer.
    GLenum get_internal_format() const { return m_args.internal_format; }

private:
    /// Checks, whether the given format is a valid internal format for a color render buffer.
    /// @throws runtime_error   ...if it isn't.
    static void _assert_color_format(const GLenum get_internal_format);

    /// Checks, whether the given format is a valid internal format for a depth or stencil render buffer.
    /// @throws runtime_error  ...if it isn't.
    static void _assert_depth_stencil_format(const GLenum get_internal_format);

    /// Deallocates the RenderBuffer data and invalidates the RenderBuffer.
    void _deallocate();

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Render Context owning the render buffer.
    GraphicsContext& m_graphics_context;

    /// OpenGL ID of the render buffer.
    RenderBufferId m_id = 0;

    /// Arguments passed to this render buffer.
    Args m_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_RenderBuffer<FrameBuffer> {
    friend class notf::FrameBuffer;

    /// Deallocates the RenderBuffer data and invalidates the RenderBuffer.
    static void deallocate(RenderBuffer& renderbuffer) { renderbuffer._deallocate(); }
};

// ================================================================================================================== //

// TODO: read pixels from framebuffer into raw image
class FrameBuffer {

    friend class access::_FrameBuffer<GraphicsContext>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_FrameBuffer<T>;

    /// A FrameBuffer's color target can be either a RenderBuffer or a Texture.
    using ColorTarget = notf::variant<TexturePtr, RenderBufferPtr>;

    /// A FrameBuffer's depth target can be either a RenderBuffer or a Texture.
    using DepthTarget = notf::variant<TexturePtr, RenderBufferPtr>;

    /// Arguments used to initialize a Texture.
    /// If you want to set both depth- and stencil targets, both have to refer to the same RenderBuffer and that
    /// RenderBuffer needs a format packing both depth and stencil.
    struct Args {

        /// Defines a color target.
        /// If the id already identifies a color target, it is updated.
        /// @param id       Color target id.
        /// @param target   Color target.
        void set_color_target(const ushort get_id, ColorTarget target);

        /// All color targets
        /// A color target consists of a pair of color buffer id / render target.
        std::vector<std::pair<ushort, ColorTarget>> color_targets;

        /// Depth target.
        DepthTarget depth_target;

        /// Stencil target.
        RenderBufferPtr stencil_target;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param context          Graphics context owning the frane buffer.
    /// @param args             Frame buffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    FrameBuffer(GraphicsContext& get_context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(FrameBuffer);

    /// Factory.
    /// @param context          Graphics context owning the frane buffer.
    /// @param args             Frame buffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static FrameBufferPtr create(GraphicsContext& get_context, Args&& args);

    /// Destructor.
    ~FrameBuffer();

    /// The FrameBuffer's id.
    FrameBufferId get_id() const { return m_id; }

    /// GraphicsContext containing the frame buffer.
    GraphicsContext& get_context() { return m_context; }

    /// Texture used as color attachment.
    /// @throws runtime_error   If there is no texture attached as the color target.
    const TexturePtr& get_color_texture(const ushort get_id);

private:
    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    void _deallocate();

    /// Checks if we can create a valid frame buffer with the given arguments.
    /// @throws runtime_error   ...if we dont.
    void _validate_arguments() const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// GraphicsContext containing the FrameBuffer.
    GraphicsContext& m_context;

    /// OpenGL ID of the FrameBuffer.
    FrameBufferId m_id;

    /// Arguments passed to this FrameBuffer.
    Args m_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_FrameBuffer<GraphicsContext> {
    friend class notf::GraphicsContext;

    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    static void deallocate(FrameBuffer& framebuffer) { framebuffer._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
