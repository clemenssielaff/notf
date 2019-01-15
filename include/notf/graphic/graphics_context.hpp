#pragma once

#include <vector>

#include "notf/meta/exception.hpp"
#include "notf/meta/pointer.hpp"

#include "notf/common/aabr.hpp"
#include "notf/common/color.hpp"
#include "notf/common/mutex.hpp"

#include "notf/graphic/fwd.hpp"
#include "notf/graphic/gl_modes.hpp"

NOTF_OPEN_NAMESPACE

class Window;

// graphics context ================================================================================================= //

/// The GraphicsContext is an abstraction of the OpenGL graphics context.
/// It is the object owning all NoTF graphics objects like shaders and textures.
class GraphicsContext {

    friend Accessor<GraphicsContext, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(GraphicsContext);

    /// Buffers to clear in a call to `clear`.
    enum Buffer : unsigned char {
        COLOR = 1u << 1,  //< Color buffer
        DEPTH = 1u << 2,  //< Depth buffer
        STENCIL = 1u << 3 //< Stencil buffer
    };
    using BufferFlags = std::underlying_type<Buffer>::type;

    // context guard  ---------------------------------------------------------

    /// Guard that makes sure that an OpenGL context is properly made current and released on a thread.
    /// Blocks until the context is free to be aquired by this thread.
    struct NOTF_NODISCARD ContextGuard {
        friend class GraphicsContext;

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param context   GraphicsContext that created the guard.
        ContextGuard(GraphicsContext& context) : m_context(&context) { m_mutex_lock = m_context->_make_current(); }

    public:
        NOTF_NO_COPY_OR_ASSIGN(ContextGuard);
        NOTF_NO_HEAP_ALLOCATION(ContextGuard);

        /// Default (empty) constructor.
        ContextGuard() = default;

        /// Move Constructor.
        /// @param other        CurrentGuard to move from.
        /// @throws ThreadError When you try to move an active guard accross thread boundaries.
        ContextGuard(ContextGuard&& other) {
            if (other.m_thread_id != m_thread_id) {
                NOTF_THROW(ThreadError, "Cannot move a \"ContextGuard\" accross thread boundaries");
            }
            m_context = other.m_context;
            m_mutex_lock = std::move(other.m_mutex_lock);

            other.m_context = nullptr;
        }

        /// Destructor.
        ~ContextGuard() {
            if (m_context) { m_context->_release_current(); }
        }

        // fields -------------------------------------------------------------
    private:
        /// GraphicsContext that created the guard.
        GraphicsContext* m_context = nullptr;

        /// Mutex lock.
        std::unique_lock<RecursiveMutex> m_mutex_lock;

        /// Id of the thread in which the context is current.
        const std::thread::id m_thread_id = std::this_thread::get_id();
    };

    // program guard  ---------------------------------------------------------

    /// RAII guard to make sure that a bound ShaderProgram is always properly unbound after use.
    /// You can nest multiple guards, each will restore the previously bound ShaderProgram.
    class NOTF_NODISCARD ProgramGuard {
        friend class GraphicsContext;

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param context  Context that created the guard.
        /// @param program  ShaderProgram to bind and unbind.
        ProgramGuard(GraphicsContext& context, const ShaderProgramPtr& program)
            : m_context(context), m_guarded_program(program), m_previous_program(context.m_state.program) {
            context._bind_program(m_guarded_program);
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(ProgramGuard);
        NOTF_NO_HEAP_ALLOCATION(ProgramGuard);

        /// Explicit move Constructor.
        /// @param other    ProgramGuard to move from.
        ProgramGuard(ProgramGuard&& other) = default;

        /// Destructor.
        ~ProgramGuard() {
            m_context._unbind_program(m_guarded_program);
            if (m_previous_program) { m_context._bind_program(m_previous_program); }
        }

        // fields -------------------------------------------------------------
    private:
        /// Context that created the guard.
        GraphicsContext& m_context;

        /// ShaderProgram to bind and unbind.
        ShaderProgramPtr m_guarded_program;

        /// Previously bound ShaderProgram, is restored on guard destruction.
        ShaderProgramPtr m_previous_program;
    };

    // framebuffer guard  -----------------------------------------------------

    /// RAII guard to make sure that a bound FrameBuffer is always properly unbound after use.
    /// You can nest multiple guards, each will restore the previously bound FrameBuffer.
    class NOTF_NODISCARD FramebufferGuard {
        friend class GraphicsContext;

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param context      Context that created the guard.
        /// @param framebuffer  FrameBuffer to bind and unbind.
        FramebufferGuard(GraphicsContext& context, const FrameBufferPtr& framebuffer)
            : m_context(context)
            , m_guarded_framebuffer(framebuffer)
            , m_previous_framebuffer(context.m_state.framebuffer) {
            context._bind_framebuffer(m_guarded_framebuffer);
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(FramebufferGuard);
        NOTF_NO_HEAP_ALLOCATION(FramebufferGuard);

        /// Explicit move Constructor.
        /// @param other    Framebuffer to move from.
        FramebufferGuard(FramebufferGuard&& other) = default;

        /// Destructor.
        ~FramebufferGuard() {
            m_context._unbind_framebuffer(m_guarded_framebuffer);
            if (m_previous_framebuffer) { m_context._bind_framebuffer(m_previous_framebuffer); }
        }

        // fields -------------------------------------------------------------
    private:
        /// Context that created the guard.
        GraphicsContext& m_context;

        /// FrameBuffer to bind and unbind.
        FrameBufferPtr m_guarded_framebuffer;

        /// Previously bound FrameBuffer, is restored on guard destruction.
        FrameBufferPtr m_previous_framebuffer;
    };

    // vao guard  -------------------------------------------------------------

    // TODO: Proper VAO class.
    // Right now I bind a VAO without the context but others only with one / also VAOs cannot be shared, so they should
    // keep a reference to their context around to check when a context tries to make them current.

    /// RAII guard for vector array object bindings.
    class NOTF_NODISCARD VaoGuard final {

        // methods ------------------------------------------------------------
    public:
        /// Constructor.
        /// @param vao  Vertex array object ID.
        VaoGuard(GLuint vao);

        /// Destructor.
        ~VaoGuard();

        // fields -------------------------------------------------------------
    private:
        /// Vertex array object ID.
        const GLuint m_vao;
    };

    // TODO: I don't like all of these *Guards anymore, because they imply that the user doesn't know the current state
    // of the graphics pipeline and needs to restore it after each use. Instead, with a proper GraphicsSetup object, all
    // of the binding / -rebinding and unbinding should be done declaratively

private:
    /// UniformBuffers are always bound at a specific index.
    struct UniformBinding {
        /// Bound buffer.
        AnyUniformBufferPtr buffer;

        /// Index at which the buffer is bound.
        /// The byte offset into the buffer is calculated using `index * sizeof(UniformBuffer::block_t)`.
        size_t index = 0;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(GraphicsContext);

    /// Constructor.
    /// @param window           GLFW window displaying the contents of this context.
    /// @throws runtime_error   If the given window is invalid.
    /// @throws runtime_error   If another current OpenGL context exits.
    GraphicsContext(valid_ptr<GLFWwindow*> window);

    /// Destructor.
    virtual ~GraphicsContext();

    /// Returns the GraphicsContext current on this thread or the Graphics System.
    static GraphicsContext& get();

    /// Makes the GraphicsContext current on this thread.
    /// Blocks until the GraphicsContext's mutex is free.
    /// @throws ThreadError If another context is already current on this thread.
    ContextGuard make_current();

#ifdef NOTF_DEBUG
    /// Tests if the GraphicsContext is current on this thread.
    bool is_current() const { return m_mutex.is_locked_by_this_thread(); }
#endif

    /// Tests whether two GraphicsContexts are the same.
    bool operator==(const GraphicsContext& other) const { return _get_window() == other._get_window(); }

    /// Returns the size of the context's window in pixels.
    /// Note that this might not be the current render size. For example, if you are currently rendering into a
    /// FrameBuffer that has a color texture of size 128x128, the window size is most likely much larger than that.
    Size2i get_window_size() const;

    /// Area that is rendered into.
    const Aabri& get_render_area() const { return m_state.render_area; }

    /// Applies the given stencil mask.
    /// @param mask     Mask to apply.
    /// @param force    Ignore the current state and always make the OpenGL call.
    void set_stencil_mask(const GLuint mask, const bool force = false);

    /// Applies the given blend mode to OpenGL.
    /// @param mode     Blend mode to apply.
    /// @param force    Ignore the current state and always make the OpenGL call.
    void set_blend_mode(const BlendMode mode, const bool force = false);

    /// Define a new area that is rendered into.
    /// @param offset   New area.
    /// @param force    Ignore the current state and always make the OpenGL call.
    /// @throws ValueError  If the given area is invalid.
    void set_render_area(Aabri area, const bool force = false);

    /// Sets the new clear color.
    /// @param color    Color to apply.
    /// @param buffers  Buffers to clear.
    void clear(Color color, const BufferFlags buffers = Buffer::COLOR);

    /// Begins the render of a frame.
    void begin_frame();

    /// Finishes the render of a frame.
    void finish_frame();

    /// Binds the given texture at the given texture slot.
    /// Only results in an OpenGL call if the texture is not currently bound.
    /// @param texture      Texture to bind.
    /// @param slot         Texture slot to bind the texture to.
    /// @throws OpenGLError If the texture is not valid or if the slot is >= than the number of texture slots in the
    ///                     environment.
    void bind_texture(const Texture* get_texture, uint slot);
    void bind_texture(const TexturePtr& texture, uint slot) { bind_texture(texture.get(), slot); }

    /// Unbinds the current texture and clears the context's texture stack.
    /// @param slot         Texture slot to clear.
    /// @throws OpenGLError If slot is >= than the number of texture slots in the environment.
    void unbind_texture(uint slot);

    /// Binds the given Program.
    /// @param program  ShaderProgram to bind.
    /// @returns        Guard, making sure that the ShaderProgram is properly unbound and the previous one restored
    ///                 after use.
    ProgramGuard bind_program(const ShaderProgramPtr& program) { return ProgramGuard(*this, program); }

    /// Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    /// @returns Guard, making sure that the FrameBuffer is properly unbound and the previous one restored after use.
    FramebufferGuard bind_framebuffer(const FrameBufferPtr& framebuffer) {
        return FramebufferGuard(*this, framebuffer);
    }

    // uniform buffer ---------------------------------------------------------

    /// Binds the given UniformBuffer at the given slot, if it is not already bound.
    /// @param slot             Uniform buffer binding slot.
    /// @param uniform_buffer   UniformBuffer to bind.
    /// @param index            Index of the UniformBuffer to bind.
    /// @throws OpenGLError     If slot is >= the maximal number of available uniform buffer slots.
    /// @throws IndexError      If index is >= the number of blocks in the UniformBuffer.
    void bind_uniform_buffer(const uint slot, const AnyUniformBufferPtr& uniform_buffer, const size_t index);

    /// Unbinds the current UniformBuffer from the given slot.
    /// @param slot             Uniform buffer binding slot.
    /// @param uniform_buffer   Only unbind the current UniformBuffer, if it is equal to the one given, noop otherwise.
    ///                         If empty (the default), the current UniformBuffer is always unbound.
    /// @throws OpenGLError     If slot is >= the maximal number of available uniform buffer slots.
    void unbind_uniform_buffer(const uint slot, const AnyUniformBufferPtr& uniform_buffer = {});

    // methods --------------------------------------------------------------------------------- //
protected:
    /// The GLFW window owning the associated OpenGL context.
    GLFWwindow* _get_window() const { return m_window; }

    /// Shuts the GraphicsContext down for good.
    void _shutdown() {
        std::call_once(m_was_closed, [&] { _shutdown_once(); });
    }

    /// Shut down implementation
    virtual void _shutdown_once();

private:
    /// Makes the GraphicsSystem's OpenGL context current on the current thread.
    /// Does nothing if the context is already current.
    /// @returns Mutex lock.
    std::unique_lock<RecursiveMutex> _make_current();

    /// Decreases the recursion count of this the GraphicsSystem's context.
    void _release_current();

    /// Unbinds bound texures from all slots.
    void _unbind_all_textures();

    /// Create a new State.
    void _reset_state();

    // program ----------------------------------------------------------------

    /// Binds the given ShaderProgram, if it is not already bound.
    /// @param program  ShaderProgram to bind.
    void _bind_program(const ShaderProgramPtr& program);

    /// Unbinds the current ShaderProgram.
    /// @param program  Only unbind the current ShaderProgram, if it is equal to the one given. If empty (the default),
    ///                 the current ShaderProgram is always unbound.
    void _unbind_program(const ShaderProgramPtr& program = {});

    // framebuffer ------------------------------------------------------------

    /// Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    void _bind_framebuffer(const FrameBufferPtr& framebuffer);

    /// Unbinds the current FrameBuffer.
    /// @param framebuffer  Only unbind the current FrameBuffer, if it is equal to the one given. If empty (the
    ///                     default), the current FrameBuffer is always unbound.
    void _unbind_framebuffer(const FrameBufferPtr& framebuffer = {});

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Flag to indicate whether the GraphicsContext has already been closed.
    std::once_flag m_was_closed;

private:
    /// The GLFW window owning the associated OpenGL context.
    GLFWwindow* m_window;

    /// Mutex to make sure that only one thread is accessing the GraphicsSystem's OpenGL context at any time.
    RecursiveMutex m_mutex;

    /// Number of times the mutex was locked.
    int m_recursion_counter = 0;

    /// The current state of the context.
    struct State {

        /// Blend mode.
        BlendMode blend_mode = BlendMode::DEFAULT;

        /// Culling.
        CullFace cull_face = CullFace::DEFAULT;

        /// Stencil mask.
        GLuint stencil_mask = 0xffffffff;

        /// Bound textures.
        std::vector<TextureConstPtr> texture_slots;

        /// Bound ShaderProgram.
        ShaderProgramPtr program;

        /// Bound Framebuffer.
        FrameBufferPtr framebuffer;

        /// Bound UniformBuffer
        std::vector<UniformBinding> uniform_buffers;

        /// Color applied when the bound framebuffer is cleared.
        Color clear_color = Color::black();

        /// On-screen AABR that is rendered into.
        Aabri render_area;
    } m_state;
};

// graphics context accessor ======================================================================================== //

template<>
class Accessor<GraphicsContext, Window> {
    friend class Window;

    /// Shuts the GraphicsContext down for good.
    static void shutdown(GraphicsContext& context) { context._shutdown(); }
};

NOTF_CLOSE_NAMESPACE
