#pragma once

#include <variant>
#include <vector>

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/size2.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// render buffer ==================================================================================================== //

/// Base class for all RenderBuffers (color, depth and stencil).
/// RenderBuffers are not managed by the GraphicsContext, but by FrameBuffers instead.
/// However, if the underlying GraphicContext should be destroyed, all RenderBuffer objects are deallocated as well.
class RenderBuffer {

    friend Accessor<RenderBuffer, FrameBuffer>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(RenderBuffer);

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

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(RenderBuffer);

    /// Constructor.
    /// @param args             Render buffer arguments.
    /// @throws ValueError      If the arguments fail to validate.
    /// @throws OpenGLError     If the RenderBuffer could not be allocated.
    RenderBuffer(Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderBuffer);

    /// Factory.
    /// @param args            Render buffer arguments.
    /// @throws runtime_error  If the arguments fail to validate.
    static RenderBufferPtr create(Args&& args) { return _create_shared(std::move(args)); }

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

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// OpenGL ID of the render buffer.
    RenderBufferId m_id = 0;

    /// Arguments passed to this render buffer.
    Args m_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<RenderBuffer, FrameBuffer> {
    friend class notf::FrameBuffer;

    /// Deallocates the RenderBuffer data and invalidates the RenderBuffer.
    static void deallocate(RenderBuffer& renderbuffer) { renderbuffer._deallocate(); }
};

// frame buffer ===================================================================================================== //

// TODO: read pixels from framebuffer into raw image
class FrameBuffer {

    friend Accessor<FrameBuffer, TheGraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(FrameBuffer);

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
        void set_color_target(const ushort get_id, ColorTarget target);

        /// All color targets
        /// A color target consists of a pair of color buffer id / render target.
        std::vector<std::pair<ushort, ColorTarget>> color_targets;

        /// Depth target.
        DepthTarget depth_target;

        /// Stencil target.
        RenderBufferPtr stencil_target;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(FrameBuffer);

    /// Constructor.
    /// @param args             Frame buffer arguments.
    /// @throws ValueError      If the arguments fail to validate.
    /// @throws OpenGLError     If the RenderBuffer could not be allocated.
    FrameBuffer(Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(FrameBuffer);

    /// Factory.
    /// @param args             Frame buffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static FrameBufferPtr create(Args&& args);

    /// Destructor.
    ~FrameBuffer();

    /// The FrameBuffer's id.
    FrameBufferId get_id() const { return m_id; }

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
    /// OpenGL ID of the FrameBuffer.
    FrameBufferId m_id;

    /// Arguments passed to this FrameBuffer.
    Args m_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<FrameBuffer, TheGraphicsSystem> {
    friend class notf::TheGraphicsSystem;

    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    static void deallocate(FrameBuffer& framebuffer) { framebuffer._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
