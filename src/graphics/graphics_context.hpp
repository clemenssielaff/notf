#pragma once

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "app/forwards.hpp"
#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "graphics/forwards.hpp"
#include "graphics/gl_modes.hpp"
#include "graphics/ids.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _GraphicsContext;
} // namespace access

// ================================================================================================================== //

/// The GraphicsContext is an abstraction of the OpenGL graphics context.
/// It is the object owning all NoTF graphics objects like shaders and textures.
class GraphicsContext {

    friend class access::_GraphicsContext<Texture>;
    friend class access::_GraphicsContext<Shader>;
    friend class access::_GraphicsContext<FrameBuffer>;
    friend class access::_GraphicsContext<Pipeline>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_GraphicsContext<T>;

    // ============================================================================================================== //

    /// Helper struct that can be used to test whether selected extensions are available in the OpenGL ES driver.
    /// Only tests for extensions on first instantiation.
    class Extensions {
        friend class GraphicsContext;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        /// Default constructor.
        Extensions();

        // fields --------------------------------------------------------------------------------------------------- //
    public:
        /// Is anisotropic filtering of textures supported?
        /// @see    https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
        bool anisotropic_filter;

        /// Does the GPU support GPU shader5 extensions?
        /// @see   https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_gpu_shader5.txt
        bool nv_gpu_shader5;
    };

    // ============================================================================================================== //

    /// Helper struct containing variables that need to be read from OpenGL at runtime and won't change over the
    /// course of the app.
    class Environment {
        friend class GraphicsContext;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        /// Default constructor.
        Environment();

        // fields --------------------------------------------------------------------------------------------------- //
    public:
        /// Maximum height and width of a render buffer in pixels.
        GLuint max_render_buffer_size;

        /// Number of available color attachments for a frame buffer.
        GLuint color_attachment_count;

        /// Number of texture slots, meaning the highest valid slot is texture_slot_count - 1.
        /// This number will be less than the actual number of texture slots available on the machine, because it
        /// subtracts slots used for internal purposed (the font atlas texture, for example).
        GLuint texture_slot_count;

        /// Texture slot reserved for the font atlas texture.
        /// Note that this is the slot number, not the enum value corresponding to the slot.
        /// In order to get that use:
        ///     GL_TEXTURE0 + font_atlas_texture_slot
        GLuint font_atlas_texture_slot;
    };

    // ============================================================================================================== //

    /// Buffers to clear in a call to `clear`.
    enum Buffer : unsigned char {
        COLOR = 1u << 1,  //< Color buffer
        DEPTH = 1u << 2,  //< Depth buffer
        STENCIL = 1u << 3 //< Stencil buffer
    };
    using BufferFlags = std::underlying_type<Buffer>::type;

    // ============================================================================================================== //

    /// RAII guard to make sure that a bound Pipeline is always properly unbound after use.
    /// You can nest multiple guards, each will restore the previously bound pipeline.
    class NOTF_NODISCARD PipelineGuard {
        friend class GraphicsContext;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        /// Constructor.
        /// @param context  Context that created the guard.
        /// @param pipeline Pipeline to bind and unbind.
        PipelineGuard(GraphicsContext& context, const PipelinePtr& pipeline)
            : m_context(context), m_guarded_pipeline(pipeline), m_previous_pipeline(context.m_state.pipeline)
        {
            context._bind_pipeline(m_guarded_pipeline);
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(PipelineGuard);
        NOTF_NO_HEAP_ALLOCATION(PipelineGuard);

        /// Explicit move Constructor.
        /// @param other    Pipeline to move from.
        PipelineGuard(PipelineGuard&& other)
            : m_context(other.m_context)
            , m_guarded_pipeline(std::move(other.m_guarded_pipeline))
            , m_previous_pipeline(std::move(other.m_previous_pipeline))
        {
            other.m_guarded_pipeline.reset();
            other.m_previous_pipeline.reset();
        }

        /// Destructor.
        ~PipelineGuard()
        {
            m_context._unbind_pipeline(m_guarded_pipeline);
            if (m_previous_pipeline) {
                m_context._bind_pipeline(m_previous_pipeline);
            }
        }

        // fields --------------------------------------------------------------------------------------------------- //
    private:
        /// Context that created the guard.
        GraphicsContext& m_context;

        /// Pipeline to bind and unbind.
        PipelinePtr m_guarded_pipeline;

        /// Previously bound Pipeline, is restored on guard destruction.
        PipelinePtr m_previous_pipeline;
    };

    // ============================================================================================================== //

    /// RAII guard to make sure that a bound FrameBuffer is always properly unbound after use.
    /// You can nest multiple guards, each will restore the previously bound pipeline.
    class NOTF_NODISCARD FramebufferGuard {
        friend class GraphicsContext;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        /// Constructor.
        /// @param context      Context that created the guard.
        /// @param framebuffer  FrameBuffer to bind and unbind.
        FramebufferGuard(GraphicsContext& context, const FrameBufferPtr& framebuffer)
            : m_context(context)
            , m_guarded_framebuffer(framebuffer)
            , m_previous_framebuffer(context.m_state.framebuffer)
        {
            context._bind_framebuffer(m_guarded_framebuffer);
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(FramebufferGuard);
        NOTF_NO_HEAP_ALLOCATION(FramebufferGuard);

        /// Explicit move Constructor.
        /// @param other    Framebuffer to move from.
        FramebufferGuard(FramebufferGuard&& other)
            : m_context(other.m_context)
            , m_guarded_framebuffer(std::move(other.m_guarded_framebuffer))
            , m_previous_framebuffer(std::move(other.m_previous_framebuffer))
        {
            other.m_guarded_framebuffer.reset();
            other.m_previous_framebuffer.reset();
        }

        /// Destructor.
        ~FramebufferGuard()
        {
            m_context._unbind_framebuffer(m_guarded_framebuffer);
            if (m_previous_framebuffer) {
                m_context._bind_framebuffer(m_previous_framebuffer);
            }
        }

        // fields --------------------------------------------------------------------------------------------------- //
    private:
        /// Context that created the guard.
        GraphicsContext& m_context;

        /// FrameBuffer to bind and unbind.
        FrameBufferPtr m_guarded_framebuffer;

        /// Previously bound FrameBuffer, is restored on guard destruction.
        FrameBufferPtr m_previous_framebuffer;
    };

    // ============================================================================================================== //

    /// Guard that makes sure that a GraphicsContext is properly made current and released on a thread.
    /// Blocks until the GraphicsContext is free to be aquired by this thread.
    struct NOTF_NODISCARD CurrentGuard {
        friend class GraphicsContext;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        /// Constructor.
        /// @param context      Context that created the guard.
        CurrentGuard(GraphicsContext& context) : m_context(&context) { m_mutex_lock = m_context->_make_current(); }

    public:
        NOTF_NO_COPY_OR_ASSIGN(CurrentGuard);
        NOTF_NO_HEAP_ALLOCATION(CurrentGuard);

        // Default constructor.
        CurrentGuard() = default;

        /// Explicit move Constructor.
        /// @param other            CurrentGuard to move from.
        /// @throws internal_error  When you try to move an active guard accross thread boundaries.
        CurrentGuard(CurrentGuard&& other)
        {
            if (other.m_thread_id != m_thread_id) {
                NOTF_THROW(internal_error, "Cannot move a GraphicsContext::CurrentGuard accross thread boundaries");
            }
            m_context = other.m_context;
            m_mutex_lock = std::move(other.m_mutex_lock);

            other.m_context = nullptr;
        }

        /// Destructor.
        ~CurrentGuard()
        {
            if (m_context) {
                m_context->_release_current();
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Context that created the guard.
        GraphicsContext* m_context = nullptr;

        /// Mutex lock.
        std::unique_lock<RecursiveMutex> m_mutex_lock;

        /// Id of the thread in which the context is current.
        const std::thread::id m_thread_id = std::this_thread::get_id();
    };

    // ============================================================================================================== //

private:
    /// Graphics state.
    struct State {

        /// Blend mode.
        BlendMode blend_mode = BlendMode::DEFAULT;

        /// Culling.
        CullFace cull_face = CullFace::DEFAULT;

        /// Stencil mask.
        GLuint stencil_mask = 0xffffffff;

        /// Bound textures.
        std::vector<TextureConstPtr> texture_slots = {};

        /// Bound Pipeline.
        PipelinePtr pipeline = {};

        /// Bound Framebuffer.
        FrameBufferPtr framebuffer = {};

        /// Color applied when the bound framebuffer is cleared.
        Color clear_color = Color::black();

        /// On-screen AABR that is rendered into.
        Aabri render_area;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(GraphicsContext);

    /// Constructor.
    /// @param window           GLFW window displaying the contents of this context.
    /// @throws runtime_error   If the given window is invalid.
    /// @throws runtime_error   If another current OpenGL context exits.
    GraphicsContext(GLFWwindow* window);

    /// Destructor.
    ~GraphicsContext();

    /// Tests whether two Graphics Contexts are the same.
    bool operator==(const GraphicsContext& other) const { return m_window == other.m_window; }

    /// Creates and returns an GLExtension instance.
    static const Extensions& get_extensions();

    /// Creates and initializes information about the graphics environment.
    static const Environment& get_environment();

    /// Returns the size of the context's window in pixels.
    /// Note that this might not be the current render size. For example, if you are currently rendering into a
    /// FrameBuffer that has a color texture of size 128x128, the window size is most likely much larger than that.
    Size2i get_window_size() const;

    /// Area that is rendered into.
    const Aabri& get_render_area() const { return m_state.render_area; }

    ///@{
    /// FontManager used to render text.
    FontManager& get_font_manager() { return *m_font_manager; }
    const FontManager& get_font_manager() const { return *m_font_manager; }
    ///@}
    /// En- or disables vsync (enabled by default).
    /// @param enabled  Whether to enable or disable vsync.
    void set_vsync(const bool enabled);

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
    /// @throws runtime_error   If the given area is invalid.
    void set_render_area(Aabri area, const bool force = false);

    /// Sets the new clear color.
    /// @param color    Color to apply.
    /// @param buffers  Buffers to clear.
    void clear(Color color, const BufferFlags buffers = Buffer::COLOR);

    /// Makes this context current on this thread.
    /// Blocks until the GraphicsContext's mutex is free.
    CurrentGuard make_current() { return CurrentGuard(*this); }
    // TODO: this has the potential of a deadlock!
    // The RenderThread needs to lock the scene hierarchy mutex from time to time - which can also be held by the event
    // handler thread. If the event handler thread asks for the GraphicsContext to become current, the two will wait
    // for each other indefinetely

    /// Begins the render of a frame.
    void begin_frame();

    /// Finishes the render of a frame.
    void finish_frame();

    // texture ----------------------------------------------------------------

    /// Checks whether this context contains a Texture with the given ID.
    /// @param id   ID of the Texture.
    bool has_texture(const TextureId& id) { return m_textures.count(id) != 0; }

    /// Finds and returns a Texture of this context by its name.
    /// @param id               ID of the Texture.
    /// @throws out_of_range    If the context does not contain a Texture with the given id.
    TexturePtr get_texture(const TextureId& id) const;

    /// Binds the given texture at the given texture slot.
    /// Only results in an OpenGL call if the texture is not currently bound.
    /// @param texture          Texture to bind.
    /// @param slot             Texture slot to bind the texture to.
    /// @throws runtime_error   If the texture is not valid.
    /// @throws runtime_error   If slot is >= than the number of texture slots in the environment.
    void bind_texture(const Texture* get_texture, uint slot);
    void bind_texture(const TexturePtr& texture, uint slot) { bind_texture(texture.get(), slot); }

    /// Unbinds the current texture and clears the context's texture stack.
    /// @param slot             Texture slot to clear.
    /// @throws runtime_error   If slot is >= than the number of texture slots in the environment.
    void unbind_texture(uint slot);

    // shader -----------------------------------------------------------------

    /// Checks whether this context contains a Shader with the given ID.
    /// @param id   ID of the Shader.
    bool has_shader(const ShaderId& id) { return m_shaders.count(id) != 0; }

    /// Finds and returns a Shader of this context by its ID.
    /// @param id               ID of the Shader.
    /// @throws out_of_range    If the context does not contain a Shader with the given id.
    ShaderPtr get_shader(const ShaderId& id) const;

    // pipeline ---------------------------------------------------------------

    /// Checks whether this context contains a Pipeline with the given ID.
    /// @param id   ID of the Pipeline.
    bool has_pipeline(const PipelineId& id) { return m_pipelines.count(id) != 0; }

    /// Binds the given Pipeline.
    /// @param pipeline Pipeline to bind.
    /// @returns Guard, making sure that the Pipline is properly unbound and the previous one restored after use.
    PipelineGuard bind_pipeline(const PipelinePtr& pipeline);

    // framebuffer ------------------------------------------------------------

    /// Checks whether this context contains a FrameBuffer with the given ID.
    /// @param id   ID of the FrameBuffer.
    bool has_framebuffer(const FrameBufferId& id) { return m_framebuffers.count(id) != 0; }

    /// Finds and returns a FrameBuffer of this context by its ID.
    /// @param id               ID of the FrameBuffer.
    /// @throws out_of_range    If the context does not contain a FrameBuffer with the given id.
    FrameBufferPtr get_framebuffer(const FrameBufferId& id) const;

    /// Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    /// @returns Guard, making sure that the FrameBuffer is properly unbound and the previous one restored after use.
    FramebufferGuard bind_framebuffer(const FrameBufferPtr& get_framebuffer);

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Makes the OpenGL context current on the current thread.
    /// Does nothing if the context is already current.
    /// @returns Mutex lock.
    std::unique_lock<RecursiveMutex> _make_current();

    /// Decreases the recursion count of this graphics context.
    void _release_current();

    /// Unbinds bound texures from all slots.
    void _unbind_all_textures();

    /// Create a new State.
    State _create_state() const;

    // pipeline ---------------------------------------------------------------

    /// Binds the given Pipeline, if it is not already bound.
    /// @param pipeline Pipeline to bind.
    void _bind_pipeline(const PipelinePtr& pipeline);

    /// Unbinds the current Pipeline.
    /// @param pipeline Only unbind the current Pipeline, if it is equal to the one given. If empty (default), the
    ///                 current Pipeline is always unbound.
    void _unbind_pipeline(const PipelinePtr& pipeline = {});

    // framebuffer ------------------------------------------------------------

    /// Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    void _bind_framebuffer(const FrameBufferPtr& get_framebuffer);

    /// Unbinds the current FrameBuffer.
    /// @param framebuffer  Only unbind the current FrameBuffer, if it is equal to the one given. If empty (default),
    /// the
    ///                     current FrameBuffer is always unbound.
    void _unbind_framebuffer(const FrameBufferPtr& get_framebuffer = {});

    // registration -----------------------------------------------------------

    /// Registers a new Texture with this GraphicsContext.
    /// @param texture          New Texture to register.
    /// @throws internal_error  If another Texture with the same ID already exists.
    void _register_new(TexturePtr texture);

    /// Registers a new Shader with this GraphicsContext.
    /// @param shader           New Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    void _register_new(ShaderPtr shader);

    /// Registers a new FrameBuffer with this GraphicsContext.
    /// @param framebuffer      New FrameBuffer to register.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    void _register_new(FrameBufferPtr framebuffer);

    /// Registers a new Pipeline with this GraphicsContext.
    /// @param pipeline         New Pipeline to register.
    /// @throws internal_error  If another Pipeline with the same ID already exists.
    void _register_new(PipelinePtr pipeline);

    /// Call this function after the last shader has been compiled.
    /// Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
    /// guaranteed to do so.
    /// If you compile a new shader after calling this function, the driver will reallocate the compiler.
    void release_shader_compiler();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The GLFW window displaying the contents of this context.
    GLFWwindow* m_window;

    /// The current state of the context.
    State m_state;

    /// True if this context has vsync enabled.
    bool m_has_vsync = true;

    /// The FontManager associated with this context.
    FontManagerPtr m_font_manager;

    // thread-safety ----------------------------------------------------------

    // TODO: this whole thread-safety of the graphics context is not very well thought out...

    /// Mutex to make sure that only one thread is accessing the render context at any time.
    RecursiveMutex m_mutex;

    /// Number of times the mutex was locked.
    int m_recursion_counter = 0;

    // resources --------------------------------------------------------------

    /// All Textures managed by this Context.
    /// Note that the Context doesn't "own" the textures, they are shared pointers, but the Render Context deallocates
    /// all Textures when it is deleted.
    std::unordered_map<TextureId, TextureWeakPtr> m_textures;

    /// All Shaders managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<ShaderId, ShaderWeakPtr> m_shaders;

    /// All FrameBuffers managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<FrameBufferId, FrameBufferWeakPtr> m_framebuffers;

    /// All Pipelines managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<PipelineId, PipelineWeakPtr> m_pipelines;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_GraphicsContext<Texture> {
    friend class notf::Texture;

    /// Registers a new Texture.
    /// @param context          GraphicsContext to access.
    /// @param texture          New Texture to register.
    /// @throws internal_error  If another Texture with the same ID already exists.
    static void register_new(GraphicsContext& context, TexturePtr texture)
    {
        context._register_new(std::move(texture));
    }
};

template<>
class access::_GraphicsContext<Shader> {
    friend class notf::Shader;

    /// Registers a new Shader.
    /// @param context          GraphicsContext to access.
    /// @param shader           New Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static void register_new(GraphicsContext& context, ShaderPtr shader) { context._register_new(std::move(shader)); }
};

template<>
class access::_GraphicsContext<FrameBuffer> {
    friend class notf::FrameBuffer;

    /// Registers a new FrameBuffer.
    /// @param context          GraphicsContext to access.
    /// @param framebuffer      New FrameBuffer to register.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static void register_new(GraphicsContext& context, FrameBufferPtr framebuffer)
    {
        context._register_new(std::move(framebuffer));
    }
};

template<>
class access::_GraphicsContext<Pipeline> {
    friend class notf::Pipeline;

    /// Registers a new Pipeline.
    /// @param context          GraphicsContext to access.
    /// @param pipeline         New Pipeline to register.
    /// @throws internal_error  If another pipeline with the same ID already exists.
    static void register_new(GraphicsContext& context, PipelinePtr pipeline)
    {
        context._register_new(std::move(pipeline));
    }
};

NOTF_CLOSE_NAMESPACE
