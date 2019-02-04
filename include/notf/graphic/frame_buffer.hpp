#pragma once

#include <variant>
#include <vector>

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/size2.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// render buffer ==================================================================================================== //

/// All types of RenderBuffers (color, depth and stencil).
///
/// RenderBuffer Objects are OpenGL Objects that contain images and are used specifically with Framebuffers. They are
/// optimized for use as render targets, while Textures may not be, and are the logical choice when you do not need to
/// sample (i.e. in a post-pass shader) from the produced image. If you need to resample (such as when reading depth
/// back in a second shader pass), use Textures instead.
///
/// RenderBuffers are shared among all GraphicContexts and managed through `shared_ptr`s. When TheGraphicSystem goes
/// out of scope, all RenderBuffers will be deallocated. Trying to modify a deallocated RenderBuffer will throw an
/// exception.
class RenderBuffer {

    friend Accessor<RenderBuffer, detail::GraphicsSystem>;

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
        Size2s size = Size2s::zero();

        /// Internal value format of a pixel in the buffer.
        GLenum internal_format = 0;

        /// Number of multisamples. 0 means no multisampling.
        GLsizei samples = 0;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(RenderBuffer);

    /// Constructor.
    /// @param name         Proposed name of this RenderBuffer.
    /// @param args         Render buffer arguments.
    /// @throws ValueError  If the arguments fail to validate.
    /// @throws OpenGLError If the RenderBuffer could not be allocated.
    RenderBuffer(std::string name, Args args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderBuffer);

    /// Factory.
    /// @param name         Proposed name of this RenderBuffer.
    /// @param args         Render buffer arguments.
    /// @throws ValueError  If the arguments fail to validate.
    /// @throws OpenGLError If the RenderBuffer could not be allocated.
    static RenderBufferPtr create(std::string name, Args args);

    /// Destructor.
    ~RenderBuffer();

    /// Name of this RenderBuffer under which it is acessible from the ResourceManager.
    const std::string& get_name() const { return m_name; }

    /// OpenGL ID of the render buffer.
    RenderBufferId get_id() const { return m_id; }

    /// Checks if this RenderBuffer is still valid or if it has been deallocated.
    bool is_valid() const { return m_id.is_valid(); }

    /// Buffer type.
    Type get_type() const { return m_args.type; }

    /// Size of the render buffer in pixels.
    const Size2s& get_size() const { return m_args.size; }

    /// Internal value format of a pixel in the buffer.
    GLenum get_internal_format() const { return m_args.internal_format; }

private:
    /// Deallocates the RenderBuffer data and invalidates the RenderBuffer.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Name of this RenderBuffer under which it is acessible from the ResourceManager.
    std::string m_name;

    /// OpenGL ID of the render buffer.
    RenderBufferId m_id = RenderBufferId::invalid();

    /// Arguments passed to this render buffer.
    Args m_args;
};

// frame buffer ===================================================================================================== //

/// FrameBuffer objects allow one to render to non-Default Framebuffer locations, and thus render without disturbing the
/// main screen.
/// FrameBuffers are owned by a GraphicContext, and managed by the user through `shared_ptr`s. The
/// FrameBuffer is deallocated when the last `shared_ptr` goes out of scope or the associated GraphicsContext is
/// deleted, whatever happens first. Trying to modify a `shared_ptr` to a deallocated FrameBuffer will throw an
/// exception.
class FrameBuffer : public std::enable_shared_from_this<FrameBuffer> {

    friend Accessor<FrameBuffer, GraphicsContext>;

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
    /// @param context          GraphicsContext managing this FrameBuffer.
    /// @param name             Proposed name of this FrameBuffer.
    /// @param args             Frame buffer arguments.
    /// @param is_default       Is set only for the special "DefaultFramebuffer" inherent in each GraphicsContext.
    /// @throws ValueError      If the arguments fail to validate.
    /// @throws OpenGLError     If the RenderBuffer could not be allocated.
    FrameBuffer(GraphicsContext& context, std::string name, Args args, bool is_default = false);

public:
    NOTF_NO_COPY_OR_ASSIGN(FrameBuffer);

    /// Factory.
    /// @param context          GraphicsContext managing this FrameBuffer.
    /// @param args             FrameBuffer arguments.
    /// @throws runtime_error   If the arguments fail to validate.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static FrameBufferPtr create(GraphicsContext& context, std::string name, Args args);

    /// Destructor.
    ~FrameBuffer();

    /// Checks if the FrameBuffer is valid.
    /// If the GraphicsContext managing this FrameBuffer is destroyed while there is still one or more`shared_ptr`s to
    /// this FrameBuffer, the FrameBuffer will become invalid and all attempts of modification will throw an exception.
    bool is_valid() const;

    /// Name of this FrameBuffer under which it is acessible from the ResourceManager.
    const std::string& get_name() const { return m_name; }

    /// The FrameBuffer's id.
    FrameBufferId get_id() const { return m_id; }

    /// Texture used as color attachment.
    /// @throws RunTimeError    If there is no texture attached as the color target.
    const TexturePtr& get_color_texture(const ushort get_id);
    // TODO: read pixels from framebuffer into raw image

private:
    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    void _deallocate();

    /// Checks if we can create a valid frame buffer with the given arguments.
    /// @throws runtime_error   ...if we dont.
    void _validate_arguments() const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// GraphicsContext managing this Framebuffer.
    GraphicsContext& m_context;

    /// Name of this FrameBuffer under which it is acessible from the ResourceManager.
    std::string m_name;

    /// OpenGL ID of the FrameBuffer.
    FrameBufferId m_id = FrameBufferId::invalid();

    /// Whether this is the GraphicsContext's default FrameBuffer
    bool m_is_default;

    /// Arguments passed to this FrameBuffer.
    Args m_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<RenderBuffer, detail::GraphicsSystem> {
    friend class detail::GraphicsSystem;

    /// Deallocates the RenderBuffer data and invalidates the RenderBuffer.
    static void deallocate(RenderBuffer& renderbuffer) { renderbuffer._deallocate(); }
};

template<>
class Accessor<FrameBuffer, GraphicsContext> {
    friend GraphicsContext;

    /// Creates the default FrameBuffer for the given GraphicsContext.
    static FrameBufferPtr create_default(GraphicsContext& context);

    /// Deallocates the framebuffer data and invalidates the Framebuffer.
    static void deallocate(FrameBuffer& framebuffer) { framebuffer._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
