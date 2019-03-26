#pragma once

#include <map>
#include <optional>
#include <vector>

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/geo/aabr.hpp"
#include "notf/common/color.hpp"
#include "notf/common/mutex.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// graphics context ================================================================================================= //

/// The GraphicsContext is an abstraction of an OpenGL context.
/// It represents the OpenGL state machine and is used primarily by other classes in the `graphics` module.
///
/// The internal design of the GraphicsContext is a collection of hidden classes, each representing one aspect of the
/// OpenGL state machine.
class GraphicsContext {

    friend Accessor<GraphicsContext, FrameBuffer>;
    friend Accessor<GraphicsContext, ShaderProgram>;
    friend Accessor<GraphicsContext, VertexObject>;
    friend Accessor<GraphicsContext, Texture>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(GraphicsContext);

    // guard  -----------------------------------------------------------------

    /// Guard that makes sure that an OpenGL context is properly made current and released on a thread.
    /// Blocks until the context is free to be aquired by this thread.
    struct NOTF_NODISCARD Guard {
        friend class GraphicsContext;

        // methods --------------------------------------------------------- //
    private:
        /// Value constructor.
        /// @param context  GraphicsContext that created the guard.
        /// @param lock     Lock owning the GraphicsContext's recursive mutex.
        Guard(GraphicsContext& context, std::unique_lock<RecursiveMutex>&& lock);

    public:
        NOTF_NO_COPY_OR_ASSIGN(Guard);

        /// Default (empty) constructor.
        Guard() = default;

        /// Move Constructor.
        /// @param other        CurrentGuard to move from.
        /// @throws ThreadError When you try to move an active guard accross thread boundaries.
        Guard(Guard&& other) {
            if (other.m_thread_id != m_thread_id) {
                NOTF_THROW(ThreadError, "Cannot move a \"GraphicsContext::Guard\" accross thread boundaries");
            }
            m_context = other.m_context;
            m_mutex_lock = std::move(other.m_mutex_lock);
            other.m_context = nullptr;
        }

        /// Destructor.
        ~Guard();

        // fields ---------------------------------------------------------- //
    private:
        /// Id of the thread on which the guard was created.
        const std::thread::id m_thread_id = std::this_thread::get_id();

        /// GraphicsContext that created the guard (empty if default constructed).
        GraphicsContext* m_context = nullptr;

        /// Mutex lock.
        std::unique_lock<RecursiveMutex> m_mutex_lock;
    };

private:
    // stencil mask -----------------------------------------------------------

    struct _StencilMask {
        NOTF_NO_COPY_OR_ASSIGN(_StencilMask);

        /// Default constructor.
        _StencilMask() = default;

        /// Assignment operator.
        /// @param mask     New stencil mask.
        void operator=(StencilMask mask);

        /// Current stencil mask.
        operator StencilMask() const { return m_mask; }

        /// Comparison operator.
        bool operator==(const GLuint& other) const { return m_mask == other; }

        // fields ---------------------------------------------------------- //
    private:
        /// Value of the stencil mask.
        StencilMask m_mask;
    };

    // blend mode -------------------------------------------------------------

    struct _BlendMode {
        NOTF_NO_COPY_OR_ASSIGN(_BlendMode);

        /// Default constructor.
        _BlendMode() = default;

        /// Assignment operator.
        /// @param mode New blend mode.
        void operator=(const BlendMode mode);

        /// Current BlendMode.
        operator BlendMode() const { return m_mode; }

        /// Comparison operator.
        bool operator==(const BlendMode& other) const { return m_mode == other; }

        // fields ---------------------------------------------------------- //
    private:
        /// Current BlendMode.
        BlendMode m_mode = BlendMode::OFF; // blending is disabled by default in OpenGL
    };

    // face culling -----------------------------------------------------------

    struct _CullFace {
        NOTF_NO_COPY_OR_ASSIGN(_CullFace);

        /// Default constructor.
        _CullFace() = default;

        /// Assignment operator.
        /// @param mode New cull face.
        void operator=(const CullFace mode);

        /// Current CullFace.
        operator CullFace() const { return m_mode; }

        /// Comparison operator.
        bool operator==(const _CullFace& other) const { return m_mode == other; }

        // fields ---------------------------------------------------------- //
    private:
        /// Current cull face.
        CullFace m_mode = CullFace::NONE; // culling is disabled by default in OpenGL
    };

    // framebuffer binding ----------------------------------------------------

    /// Generic "framebuffer" state.
    /// Either forwards to the OpenGL context's default framebuffer or a custom one, if one is bound.
    /// The default FrameBuffer is provided by the OS and represents the renderable area of the application's
    /// window. As such, we can only render to it but not modify it in any other way.
    struct _FrameBuffer {
        NOTF_NO_COPY_OR_ASSIGN(_FrameBuffer);

        /// Constructor.
        /// @param context  GraphicsContext providing the state containing this binding.
        _FrameBuffer(GraphicsContext& context) : m_context(context) {}

        /// Destructor.
        ~_FrameBuffer() { operator=(nullptr); }

        /// Assignment operator.
        /// @param framebuffer  New FrameBuffer to bind.
        /// @throws ValueError  If the GraphicsContext of the FrameBuffer does not match.
        void operator=(FrameBufferPtr framebuffer);

        /// Currently bound FrameBuffer (can be empty).
        operator FrameBufferPtr() const { return m_framebuffer; }

        /// @{
        /// Comparison operators.
        bool operator==(const FrameBuffer* framebuffer) const { return m_framebuffer.get() == framebuffer; }
        bool operator==(const FrameBuffer& framebuffer) const { return operator==(&framebuffer); }
        bool operator==(const FrameBufferPtr& framebuffer) const { return operator==(framebuffer.get()); }
        /// @}

        /// Returns the size of the FrameBuffer in pixels.
        Size2i get_size() const;

        /// Area of the FrameBuffer that is currently being rendered into.
        const Aabri& get_render_area() const;

        /// Define a new area that is rendered into.
        /// @param offset   New area.
        /// @param force    Ignore the current state and always make the OpenGL call.
        /// @throws ValueError  If the given area is invalid.
        void set_render_area(Aabri area, const bool force = false);

        /// Sets the new clear color.
        /// @param color    Color to apply.
        /// @param buffers  Buffers to clear.
        void clear(Color color, const GLBuffers buffers = GLBuffer::COLOR);

        // fields ---------------------------------------------------------- //
    private:
        /// GraphicsContext providing the state containing this binding.
        GraphicsContext& m_context;

        /// Currently bound FrameBuffer (can be empty).
        FrameBufferPtr m_framebuffer;
    };

    // shader program binding -------------------------------------------------

    struct _ShaderProgram {
        NOTF_NO_COPY_OR_ASSIGN(_ShaderProgram);

        /// Constructor.
        /// @param context  GraphicsContext providing the state containing this binding.
        _ShaderProgram(GraphicsContext& context) : m_context(context) {}

        /// Destructor.
        ~_ShaderProgram() { operator=(nullptr); }

        /// Assignment operator.
        /// @param program      New ShaderProgram to bind.
        /// @throws ValueError  If the GraphicsContext of the FrameBuffer does not match.
        void operator=(ShaderProgramPtr program);

        /// Currently bound ShaderProgram (can be empty).
        operator ShaderProgramPtr() const { return m_program; }

        /// @{
        /// Comparison operators.
        bool operator==(const ShaderProgram* program) const { return m_program.get() == program; }
        bool operator==(const ShaderProgram& program) const { return operator==(&program); }
        bool operator==(const ShaderProgramPtr& program) const { return operator==(program.get()); }
        /// @}

        // fields ---------------------------------------------------------- //
    private:
        /// GraphicsContext providing the state containing this binding.
        GraphicsContext& m_context;

        /// Bound ShaderProgram.
        ShaderProgramPtr m_program;
    };

    // vertex object binding --------------------------------------------------

    struct _VertexObject {
        NOTF_NO_COPY_OR_ASSIGN(_VertexObject);

        /// Constructor.
        /// @param context  GraphicsContext providing the state containing this binding.
        _VertexObject(GraphicsContext& context) : m_context(context) {}

        /// Destructor.
        ~_VertexObject() { operator=(nullptr); }

        /// Assignment operator.
        /// @param vertex_object    New VertexObject to bind.
        /// @throws ValueError      If the GraphicsContext of the FrameBuffer does not match.
        void operator=(VertexObjectPtr vertex_object);

        /// Currently bound VertexObject (can be empty).
        operator VertexObjectPtr() const { return m_vertex_object; }

        /// @{
        /// Comparison operators.
        bool operator==(const VertexObject* vertex_object) const { return m_vertex_object.get() == vertex_object; }
        bool operator==(const VertexObject& vertex_object) const { return operator==(&vertex_object); }
        bool operator==(const VertexObjectPtr& vertex_object) const { return operator==(vertex_object.get()); }
        /// @}

        // fields ---------------------------------------------------------- //
    private:
        /// GraphicsContext providing the state containing this binding.
        GraphicsContext& m_context;

        /// Bound VertexObject.
        VertexObjectPtr m_vertex_object;
    };

    // texture slots ----------------------------------------------------------

    /// All Texture slots of the GraphicsContext's state.
    struct _TextureSlots {

        // types ----------------------------------------------------------- //
    private:
        struct Slot {
            NOTF_NO_COPY_OR_ASSIGN(Slot);

            /// Constructor.
            /// @param slot     Slot index.
            Slot(const GLuint index) : m_index(index) {}

            /// Destructor.
            ~Slot() { operator=(nullptr); }

            /// Assignment operator.
            /// @param texture  New Texture to bind.
            void operator=(TexturePtr texture);

            /// Currently bound Texture (can be empty).
            operator TexturePtr() const { return m_texture; }

            /// @{
            /// Comparison operators.
            bool operator==(const Texture* texture) const { return m_texture.get() == texture; }
            bool operator==(const Texture& texture) const { return operator==(&texture); }
            bool operator==(const TexturePtr& texture) const { return operator==(texture.get()); }
            /// @}

            // fields ------------------------------------------------------ //
        private:
            /// Texture bound to the slot.
            TexturePtr m_texture;

            /// Slot index.
            const GLuint m_index;
        };

        // methods --------------------------------------------------------- //
    public:
        NOTF_NO_COPY_OR_ASSIGN(_TextureSlots);

        /// Default Constructor.
        _TextureSlots() = default;

        /// TextureSlot access operator.
        /// @param index        Index of the requested slot.
        /// @throws IndexError  If the given slot index is >= the number of texture slots provided by the sytem.
        Slot& operator[](const GLuint index);

        /// Remove all texture bindings.
        void clear() { m_slots.clear(); }

        // fields ---------------------------------------------------------- //
    private:
        /// All texture slots.
        std::map<GLuint, Slot> m_slots;
    };

    // uniform slots -----------------------------------------------------------

    struct _UniformSlots {

        // types ----------------------------------------------------------- //
    private:
        /// A GraphicsContext provides `GL_MAX_UNIFORM_BUFFER_BINDINGS` UniformSlots that can be bound to by a single
        /// UniformBuffer providing and multiple UniformBlocks receiving data.
        struct Slot {

            // types ------------------------------------------------------- //
        private:
            /// Every UniformSlot is bound by 0-1 UniformBuffer object.
            class BufferBinding {
                friend Slot;

                // methods ------------------------------------------------- //
            private:
                /// Constructor.
                /// @param slot     Slot index.
                BufferBinding(const GLuint slot_index) : m_slot_index(slot_index) {}

            public:
                /// Destructor.
                ~BufferBinding() { _set(nullptr); }

                /// Bound UniformBuffer.
                const AnyUniformBufferPtr& get_buffer() const { return m_buffer; }

                /// Offset at which the buffer is bound in blocks.
                /// Calculate the offset in bytes using `offset * sizeof(UniformBuffer::block_t)`.
                size_t get_offset() const { return m_offset; }

            private:
                /// Sets the UniformBuffer and offset stored in this binding.
                /// If another buffer is already bound, it will be replaced.
                /// If the same buffer is bound at another index, this call will re-bind the buffer at the new index.
                /// @param buffer   New UniformBuffer to bind.
                /// @param offset   Offset at which the buffer is bound in blocks.
                void _set(AnyUniformBufferPtr buffer, size_t offset = 0);

                // fields -------------------------------------------------- //
            private:
                /// Bound UniformBuffer.
                AnyUniformBufferPtr m_buffer;

                /// Offset at which the buffer is bound in blocks.
                /// Calculate the offset in bytes using `offset * sizeof(UniformBuffer::block_t)`.
                size_t m_offset;

                /// Slot index.
                GLuint m_slot_index;
            };

            /// Every UniformSlot can be bound to by 0-n UniformBlocks.
            struct BlockBinding {
                friend Slot;

                // methods ------------------------------------------------- //
            private:
                /// Constructor.
                /// @param slot     Slot index.
                BlockBinding(ShaderProgramConstPtr program, const GLuint block_index, const GLuint slot_index);

            public:
                /// Destructor.
                ~BlockBinding() { _set(0); }

                /// ShaderProgram containing the bound UniformBlock.
                const ShaderProgramConstPtr& get_program() const { return m_program; }

                /// Index of the UniformBlock in the ShaderProgram.
                size_t get_block_index() const { return m_block_index; }

            private:
                /// Updates the block binding, is called from both the constructor and destructor.
                void _set(const GLuint slot_index);

                // fields -------------------------------------------------- //
            private:
                /// ShaderProgram containing the bound UniformBlock.
                ShaderProgramConstPtr m_program;

                /// Index of the UniformBlock in the ShaderProgram.
                GLuint m_block_index;

                /// ShaderID of the VertexShader referred to by the bound UniformBlock.
                ShaderId m_vertex_shader_id = ShaderId::invalid();

                /// ShaderID of the FragmentShader referred to by the bound UniformBlock.
                ShaderId m_fragment_shader_id = ShaderId::invalid();
            };

            using UniformBlock = detail::UniformBlock;

            // methods ----------------------------------------------------- //
        public:
            NOTF_NO_COPY_OR_ASSIGN(Slot);

            /// Constructor.
            /// @param slot     Slot index.
            Slot(const GLuint index) : m_buffer(index) {}

            /// Destructor.
            ~Slot() { clear(); }

            /// Assignment operator for UniformBuffers.
            /// @param buffer   New UniformBuffer to bind.
            void operator=(AnyUniformBufferPtr buffer) { bind_buffer(std::move(buffer)); }

            /// Currently bound UniformBuffer (can be empty).
            const BufferBinding& get_buffer_binding() const { return m_buffer; }

            /// Returns all UniformBlock bindings.
            const std::vector<BlockBinding>& get_block_bindings() const { return m_blocks; }

            /// Removes all bindings from this UniformSlot.
            void clear() {
                remove_buffer();
                remove_blocks();
            }

            /// Remove the bound UniformBuffer from this slot. Does nothing if no buffer is bound.
            void remove_buffer() { m_buffer._set({}, 0); }

            /// Remove the bound UniformBlock/s from this slot. Does nothing if no blocks are bound.
            void remove_blocks() { m_blocks.clear(); }

            /// Bind a new UniformBuffer to this slot at the given index.
            /// If another buffer is already bound, it will be replaced.
            /// If the same buffer is bound at another index, this call will re-bind the buffer at the new index.
            /// @param buffer   Buffer to bind.
            /// @param index    Index in the UniformBuffer at which to bind (in blocks).
            void bind_buffer(AnyUniformBufferPtr buffer, size_t offset = 0) {
                m_buffer._set(std::move(buffer), offset);
            }

            /// Binds a new UniformBlock to this slot.
            /// If the same block is already bound, this does nothing.
            /// Other bound blocks are not affected.
            /// @param block    New UniformBlock to bind.
            void bind_block(const UniformBlock& block);

            // fields ------------------------------------------------------ //
        private:
            /// Bound UniformBuffer.
            BufferBinding m_buffer;

            /// Bound UniformBlocks.
            std::vector<BlockBinding> m_blocks;
        };

        // methods --------------------------------------------------------- //
    public:
        NOTF_NO_COPY_OR_ASSIGN(_UniformSlots);

        /// Default Constructor.
        _UniformSlots() = default;

        /// UniformSlot access operator.
        /// @param index        Index of the requested slot.
        /// @throws IndexError  If the given slot index is >= the number of uniform slots provided by the sytem.
        Slot& operator[](const GLuint index);

        /// Remove all texture bindings.
        void clear() { m_slots.clear(); }

        // fields ---------------------------------------------------------- //
    private:
        /// All uniform slots.
        std::map<GLuint, Slot> m_slots;
    };

    // state ------------------------------------------------------------------

    /// The combined, current State of the GraphicsContext.
    struct _State {

        // methods ------------------------------------------------------------
    private:
        friend GraphicsContext;

        /// Constructor.
        _State(GraphicsContext& context) : framebuffer(context), program(context), vertex_object(context) {}

        // fields -------------------------------------------------------------
    public:
        /// Blend mode.
        _BlendMode blend_mode;

        /// Culling.
        _CullFace cull_face;

        /// Stencil mask.
        _StencilMask stencil_mask;

        /// Bound Framebuffer.
        _FrameBuffer framebuffer;

        /// Bound ShaderProgram.
        _ShaderProgram program;

        /// Bound VertexObject.
        _VertexObject vertex_object;

        /// Bound textures.
        _TextureSlots texture_slots;

        /// Bound UniformBuffer
        _UniformSlots uniform_slots;

    private:
        friend _FrameBuffer;

        /// Color applied at the beginning of the frame when the default framebuffer is cleared.
        Color _clear_color = Color::black();

        /// On-screen AABR that is rendered into.
        Aabri _render_area;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(GraphicsContext);

    /// Constructor.
    /// @param name         Human-readable name of this GraphicsContext. Should be unique, but doesn't have to be.
    /// @param window       GLFW window displaying the contents of this context.
    /// @throws OpenGLError If the window failed to create an OpenGL context.
    GraphicsContext(std::string name, valid_ptr<GLFWwindow*> window);

    /// Destructor.
    ~GraphicsContext();

    /// Human-readable name of this GraphicsContext.
    const std::string& get_name() const { return m_name; }

    /// Tests if the GraphicsContext is current on this thread.
    bool is_current() const { return m_mutex.is_locked_by_this_thread(); }

    /// Tests whether two GraphicsContexts are the same.
    bool operator==(const GraphicsContext& other) const { return m_window == other.m_window; }

    /// Makes the GraphicsContext current on this thread.
    /// Blocks until the GraphicsContext's mutex is free.
    /// @param assume_is_current    If set to true, assume that the context is already current and log a warning if you
    ///                             need to block.
    /// @throws ThreadError If another context is already current on this thread.
    Guard make_current(bool assume_is_current = true);

    /// Begins the render of a frame.
    void begin_frame();

    /// Finishes the render of a frame.
    void finish_frame();

    /// @{
    /// Access to the GraphicsContext's state.
    /// @throws OpenGLError If the context is not current.
    const _State* operator->() const {
        if (!is_current()) {
            NOTF_THROW(OpenGLError, "Cannot access a GraphicsContext's state without the context being current");
        }
        return &m_state;
    }
    _State* operator->() { return NOTF_FORWARD_CONST_AS_MUTABLE(operator->()); }
    /// @}

    /// Reset the GraphicsContext state.
    void reset();

private:
    /// Registers a new FrameBuffer with this GraphicsContext.
    /// @param framebuffer     New FrameBuffer to register.
    /// @throws NotUniqueError  If another FrameBuffer with the same ID already exists.
    void _register_new(FrameBufferPtr framebuffer);

    /// Registers a new Program this the GraphicsContext.
    /// @param program          New Program to register.
    /// @throws NotUniqueError  If another Program with the same ID already exists.
    void _register_new(ShaderProgramPtr program);

    /// Registers a new VertexObjecs this the GraphicsContext.
    /// @param vertex_object    New VertexObjecs to register.
    /// @throws NotUniqueError  If another VertexObjecs with the same ID already exists.
    void _register_new(VertexObjectPtr vertex_object);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Human-readable name of this GraphicsContext.
    std::string m_name;

    /// The GLFW window owning the associated OpenGL context.
    GLFWwindow* m_window;

    /// Mutex to make sure that only one thread is accessing the GraphicsSystem's OpenGL context at any time.
    RecursiveMutex m_mutex;

    /// The current state of the context.
    _State m_state;

    // resources --------------------------------------------------------------

    /// All FrameBuffers managed by this GraphicsContext.
    /// See TheGraphicsSystem for details on resource management.
    std::map<FrameBufferId, FrameBufferWeakPtr> m_framebuffers;

    /// All ShaderPrograms managed by this GraphicsContext.
    /// See TheGraphicsSystem for details on resource management.
    std::map<ShaderProgramId, ShaderProgramWeakPtr> m_programs;

    /// All VertexObjecs managed by this GraphicsContext.
    /// See TheGraphicsSystem for details on resource management.
    std::map<VertexObjectId, VertexObjectWeakPtr> m_vertex_objects;
};

// graphics context accessor ======================================================================================== //

template<>
class Accessor<GraphicsContext, FrameBuffer> {
    friend FrameBuffer;

    /// Registers a new FrameBuffer.
    /// @param context          GraphicsContext to access.
    /// @param framebuffer     New FrameBuffer to register.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static void register_new(GraphicsContext& context, FrameBufferPtr framebuffer) {
        context._register_new(std::move(framebuffer));
    }
};

template<>
class Accessor<GraphicsContext, ShaderProgram> {
    friend ShaderProgram;

    /// Registers a new Program.
    /// @param context          GraphicsContext to access.
    /// @param program          New Program to register.
    /// @throws internal_error  If another Program with the same ID already exists.
    static void register_new(GraphicsContext& context, ShaderProgramPtr program) {
        context._register_new(std::move(program));
    }
};

template<>
class Accessor<GraphicsContext, VertexObject> {
    friend VertexObject;

    /// Registers a new VertexObject.
    /// @param context          GraphicsContext to access.
    /// @param vertex_object    New VertexObject to register.
    /// @throws internal_error  If another Program with the same ID already exists.
    static void register_new(GraphicsContext& context, VertexObjectPtr vertex_object) {
        context._register_new(std::move(vertex_object));
    }
};

NOTF_CLOSE_NAMESPACE
