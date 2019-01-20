#pragma once

#include <atomic>

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/graphics_context.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {
class Application;
}

// graphics system ================================================================================================== //

namespace detail {

/// The GraphicsSystem abstract a single, shared OpenGL graphics context.
/// Is a singleton.
/// Unlike the GraphicsContext class in graphics/core, the GraphicsSystem is not meant for rendering, but to be
/// used exclusively for resource management.
///
/// Resource Sharing
/// ----------------
///
/// OpenGL ES allows sharing of:
///     * vertex / index / instance buffers
///     * render buffers
///     * uniform buffers
///     * shader
///     * textures
///     * samplers
///     * syncs
/// These objects are managed by the GraphicsSystem, as it is the last context to go out of scope.
///
/// Objects which contain references to other objects (container objects) are explicitly NOT SHARED:
///     * framebuffers
///     * shader programs
///     * vertex objects
///     * queries
///     * transform feedbacks
///
class GraphicsSystem {

    friend TheGraphicsSystem;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Helper struct that can be used to test whether selected extensions are available.
    /// Only tests for extensions on first instantiation.
    class Extensions {
        friend class GraphicsSystem;

        // methods ------------------------------------------------------------
    private:
        /// Default constructor.
        Extensions();

        // fields -------------------------------------------------------------
    public:
        /// Is anisotropic filtering of textures supported?
        /// @see    https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
        bool anisotropic_filter;

        /// Does the GPU support GPU shader5 extensions?
        /// @see   https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_gpu_shader5.txt
        bool gpu_shader5;

        /// Does the GPU support negative swap intervals?
        /// @see https://www.khronos.org/registry/OpenGL/extensions/EXT/GLX_EXT_swap_control_tear.txt
        bool swap_control_tear;
    };

    // ------------------------------------------------------------------------

    /// Helper struct containing variables that need to be read from OpenGL at runtime and won't change over the
    /// course of the app.
    class Environment {
        friend class GraphicsSystem;

        // methods ------------------------------------------------------------
    private:
        /// Default constructor.
        Environment();

        // fields -------------------------------------------------------------
    public:
        /// Maximum height and width of a render buffer in pixels.
        GLuint max_render_buffer_size;

        /// Number of available color attachments for a frame buffer.
        GLuint color_attachment_count;

        /// Number of texture slots, meaning the highest valid slot is texture_slot_count - 1.
        /// This number will be less than the actual number of texture slots available on the machine, because it
        /// subtracts slots used for internal purposed (the font atlas texture, for example).
        GLuint texture_slot_count;

        /// Number of uniform slots, meaning the highest valid slot is uniform_slot_count - 1.
        GLuint uniform_slot_count;

        /// Number of supported vertex attributes.
        /// OpenGL sais there have to be at least 16.
        GLuint vertex_attribute_count;

        /// Maximum number of samples that can be specified for multisampling.
        GLint max_sample_count;

        /// Texture slot reserved for the font atlas texture.
        /// Note that this is the slot number, not the enum value corresponding to the slot.
        /// In order to get that use:
        ///     GL_TEXTURE0 + font_atlas_texture_slot
        GLuint font_atlas_texture_slot;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Creates and returns an GLExtension instance.
    static const Extensions& _get_extensions();

    /// Creates and initializes information about the graphics environment.
    static const Environment& _get_environment();

public:
    NOTF_NO_COPY_OR_ASSIGN(GraphicsSystem);

    /// Constructor.
    /// @param shared_window    A GLFW window whose context to use exclusively for managing resources.
    GraphicsSystem(valid_ptr<GLFWwindow*> shared_window);

    /// Desctructor
    ~GraphicsSystem();

    /// Ensures that the
    GraphicsContext::Guard make_current() { return m_context->make_current(); }

    ///@{
    /// FontManager used to render text.
    FontManager& get_font_manager() { return *m_font_manager; }
    const FontManager& get_font_manager() const { return *m_font_manager; }
    ///@}

    /// Call this function after the last shader has been compiled.
    /// Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
    /// guaranteed to do so.
    /// If you compile a new shader after calling this function, the driver will reallocate the compiler.
    void release_shader_compiler();

private:
    /// Method called right after initialization of the GraphicsSystem.
    /// At this point, the global GraphicsSystem singleton is available for other classes to use.
    void _post_initialization();

    /// Registers a new IndexBuffer with the GraphicsSystem.
    /// @param index_buffer     New IndexBuffer to register.
    /// @throws NotUniqueError  If another IndexBuffer with the same ID already exists.
    void _register_new(AnyIndexBufferPtr index_buffer);

    /// Registers a new Shader with the GraphicsSystem.
    /// @param shader           New Shader to register.
    /// @throws NotUniqueError  If another Shader with the same ID already exists.
    void _register_new(AnyShaderPtr shader);

    /// Registers a new UniformBuffer with the GraphicsSystem.
    /// @param uniform_buffer     New UniformBuffer to register.
    /// @throws NotUniqueError  If another UniformBuffer with the same ID already exists.
    void _register_new(AnyUniformBufferPtr uniform_buffer);

    /// Registers a new VertexBuffer with the GraphicsSystem.
    /// @param vertex_buffer    New VertexBuffer to register.
    /// @throws NotUniqueError  If another VertexBuffer with the same ID already exists.
    void _register_new(AnyVertexBufferPtr vertex_buffer);

    /// Registers a new RenderBuffer with the GraphicsSystem.
    /// @param renderbuffer     New RenderBuffer to register.
    /// @throws NotUniqueError  If another RenderBuffer with the same ID already exists.
    void _register_new(RenderBufferPtr renderbuffer);

    /// Registers a new Texture with the GraphicsSystem.
    /// @param texture          New Texture to register.
    /// @throws NotUniqueError  If another Texture with the same ID already exists.
    void _register_new(TexturePtr texture);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Shared context for resource management.
    GraphicsContextPtr m_context;

    /// The FontManager.
    FontManagerPtr m_font_manager;

    // resources --------------------------------------------------------------

    /// All Textures managed by the GraphicsSystem.
    /// Note that the GraphicsSystem doesn't "own" the textures, they are shared pointers, but the GraphicsSystem
    /// deallocates all Textures when it is deleted.
    std::map<TextureId, TextureWeakPtr> m_textures;

    /// All Shaders managed by the GraphicsSystem.
    /// See `m_textures` for details on management.
    std::map<ShaderId, AnyShaderWeakPtr> m_shaders;

    /// All RenderBuffers managed by the GraphicsSystem.
    /// See `m_textures` for details on management.
    std::map<RenderBufferId, RenderBufferWeakPtr> m_renderbuffers;
};

} // namespace detail

// the graphics system ============================================================================================== //

class TheGraphicsSystem : public ScopedSingleton<detail::GraphicsSystem> {

    friend Accessor<TheGraphicsSystem, AnyIndexBuffer>;
    friend Accessor<TheGraphicsSystem, AnyShader>;
    friend Accessor<TheGraphicsSystem, AnyUniformBuffer>;
    friend Accessor<TheGraphicsSystem, AnyVertexBuffer>;
    friend Accessor<TheGraphicsSystem, detail::Application>;
    friend Accessor<TheGraphicsSystem, RenderBuffer>;
    friend Accessor<TheGraphicsSystem, Texture>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheGraphicsSystem);

    using Extensions = typename detail::GraphicsSystem::Extensions;
    using Environment = typename detail::GraphicsSystem::Environment;

    // methods --------------------------------------------------------------------------------- //
public:
    using ScopedSingleton<detail::GraphicsSystem>::ScopedSingleton;

    /// Creates and returns an GLExtension instance.
    static const Extensions& get_extensions() { return detail::GraphicsSystem::_get_extensions(); }

    /// Creates and initializes information about the graphics environment.
    static const Environment& get_environment() { return detail::GraphicsSystem::_get_environment(); }

private:
    NOTF_CREATE_SMART_FACTORIES(TheGraphicsSystem);

    static std::unique_ptr<TheGraphicsSystem> create(valid_ptr<GLFWwindow*> shared_context) {
        auto result = _create_unique(Holder{}, shared_context);
        result->_get()._post_initialization();
        return result;
    }

    /// Registers a new something with the GraphicsSystem.
    template<class T>
    void _register_new(std::shared_ptr<T> sth) {
        _get()._register_new(std::move(sth));
    }
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<TheGraphicsSystem, AnyIndexBuffer> {
    friend AnyIndexBuffer;

    /// Registers a new IndexBuffer.
    /// @param index_buffer     New IndexBuffer to register.
    /// @throws NotUniquError   If another IndexBuffer with the same ID already exists.
    static void register_new(AnyIndexBufferPtr index_buffer) {
        TheGraphicsSystem()._register_new(std::move(index_buffer));
    }
};

template<>
class Accessor<TheGraphicsSystem, AnyShader> {
    friend AnyShader;

    /// Registers a new Shader.
    /// @param shader           New Shader to register.
    /// @throws NotUniquError   If another Shader with the same ID already exists.
    static void register_new(AnyShaderPtr shader) { TheGraphicsSystem()._register_new(std::move(shader)); }
};

template<>
class Accessor<TheGraphicsSystem, AnyUniformBuffer> {
    friend AnyUniformBuffer;

    /// Registers a new UniformBuffer.
    /// @param uniform_buffer   New UniformBuffer to register.
    /// @throws NotUniquError   If another UniformBuffer with the same ID already exists.
    static void register_new(AnyUniformBufferPtr uniform_buffer) {
        TheGraphicsSystem()._register_new(std::move(uniform_buffer));
    }
};

template<>
class Accessor<TheGraphicsSystem, AnyVertexBuffer> {
    friend AnyVertexBuffer;

    /// Registers a new VertexBuffer.
    /// @param vertex_buffer    New VertexBuffer to register.
    /// @throws NotUniquError   If another VertexBuffer with the same ID already exists.
    static void register_new(AnyVertexBufferPtr vertex_buffer) {
        TheGraphicsSystem()._register_new(std::move(vertex_buffer));
    }
};

template<>
class Accessor<TheGraphicsSystem, detail::Application> {
    friend detail::Application;

    /// Creates the ScopedSingleton holder instance of GraphicsSystem.
    static std::unique_ptr<TheGraphicsSystem> create(valid_ptr<GLFWwindow*> shared_context) {
        return TheGraphicsSystem::create(shared_context);
    }
};

template<>
class Accessor<TheGraphicsSystem, RenderBuffer> {
    friend RenderBuffer;

    /// Registers a new RenderBuffer.
    /// @param renderbuffer     New RenderBuffer to register.
    /// @throws NotUniquError   If another RenderBuffer with the same ID already exists.
    static void register_new(RenderBufferPtr renderbuffer) {
        TheGraphicsSystem()._register_new(std::move(renderbuffer));
    }
};

template<>
class Accessor<TheGraphicsSystem, Texture> {
    friend Texture;

    /// Registers a new Texture.
    /// @param texture          New Texture to register.
    /// @throws NotUniquError   If another Texture with the same ID already exists.
    static void register_new(TexturePtr texture) { TheGraphicsSystem()._register_new(std::move(texture)); }
};

NOTF_CLOSE_NAMESPACE
