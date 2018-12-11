#pragma once

#include <atomic>
#include <unordered_map>

#include "notf/graphic/graphics_context.hpp"

NOTF_OPEN_NAMESPACE

class TheApplication;

// the graphics system ============================================================================================== //

/// The GraphicsSystem abstract a single, shared OpenGL graphics context.
/// Is a singleton.
/// Unlike the GraphicsContext class in graphics/core, the GraphicsSystem is not meant for rendering, but to be
/// used exclusively for resource management.
class TheGraphicsSystem final : public GraphicsContext {

    friend Accessor<TheGraphicsSystem, TheApplication>;
    friend Accessor<TheGraphicsSystem, Texture>;
    friend Accessor<TheGraphicsSystem, Shader>;
    friend Accessor<TheGraphicsSystem, FrameBuffer>;
    friend Accessor<TheGraphicsSystem, ShaderProgram>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheGraphicsSystem);

    /// Helper struct that can be used to test whether selected extensions are available.
    /// Only tests for extensions on first instantiation.
    class Extensions {
        friend class TheGraphicsSystem;

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
        friend class TheGraphicsSystem;

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

        /// Texture slot reserved for the font atlas texture.
        /// Note that this is the slot number, not the enum value corresponding to the slot.
        /// In order to get that use:
        ///     GL_TEXTURE0 + font_atlas_texture_slot
        GLuint font_atlas_texture_slot;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Constructor.
    /// @param shared_window    A GLFW window whose context to use exclusively for managing resources.
    TheGraphicsSystem(valid_ptr<GLFWwindow*> shared_window);

    /// Static (private) function holding the actual GraphicsSystem instance.
    static TheGraphicsSystem& _instance(GLFWwindow* shared_window = nullptr) {
        static TheGraphicsSystem instance(shared_window);
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheGraphicsSystem);

    /// The singleton Graphics System instance.
    static TheGraphicsSystem& get() { return _instance(); }

    /// Desctructor
    ~TheGraphicsSystem() override;

    /// Creates and returns an GLExtension instance.
    static const Extensions& get_extensions();

    /// Creates and initializes information about the graphics environment.
    static const Environment& get_environment();

    ///@{
    /// FontManager used to render text.
    FontManager& get_font_manager() { return *m_font_manager; }
    const FontManager& get_font_manager() const { return *m_font_manager; }
    ///@}

    // texture ----------------------------------------------------------------

    /// Checks whether the GraphicsSystem contains a Texture with the given ID.
    /// @param id   ID of the Texture.
    bool has_texture(const TextureId& id) { return m_textures.count(id) != 0; }

    /// Finds and returns a Texture by its name.
    /// @param id               ID of the Texture.
    /// @throws out_of_range    If a Texture with the given id does not exist.
    TexturePtr get_texture(const TextureId& id) const;

    // shader -----------------------------------------------------------------

    /// Checks whether the GraphicsSystem contains a Shader with the given ID.
    /// @param id   ID of the Shader.
    bool has_shader(const ShaderId& id) { return m_shaders.count(id) != 0; }

    /// Finds and returns a Shader by its ID.
    /// @param id               ID of the Shader.
    /// @throws out_of_range    If a Shader with the given id does not exist
    ShaderPtr get_shader(const ShaderId& id) const;

    // program shader ---------------------------------------------------------

    /// Checks whether the GraphicsSystem contains a ProgramShader with the given ID.
    /// @param id   ID of the ProgramShader.
    bool has_program(const ShaderProgramId id) { return m_programs.count(id) != 0; }

    /// Finds and returns a ProgramShader by its ID.
    /// @param id           ID of the ProgramShader.
    /// @throws OutOfRange  If a ProgramShader with the given id does not exist
    ShaderProgramPtr get_program(const ShaderProgramId id) const;

    // framebuffer ------------------------------------------------------------

    /// Checks whether the GraphicsSystem contains a FrameBuffer with the given ID.
    /// @param id   ID of the FrameBuffer.
    bool has_framebuffer(const FrameBufferId& id) { return m_framebuffers.count(id) != 0; }

    /// Finds and returns a FrameBuffer by its ID.
    /// @param id           ID of the FrameBuffer.
    /// @throws OutOfRange  If a FrameBuffer with the given id does not exist
    FrameBufferPtr get_framebuffer(const FrameBufferId& id) const;

private:
    /// Method called right after initialization of the GraphicsSystem.
    /// At this point, the global GraphicsSystem singleton is available for other classes to use.
    void _post_initialization();

    /// Shut down implementation
    void _shutdown_once() final;

    /// Registers a new Texture with the GraphicsSystem.
    /// @param texture          New Texture to register.
    /// @throws NotUniqueError  If another Texture with the same ID already exists.
    void _register_new(TexturePtr texture);

    /// Registers a new Shader with the GraphicsSystem.
    /// @param shader           New Shader to register.
    /// @throws NotUniqueError  If another Shader with the same ID already exists.
    void _register_new(ShaderPtr shader);

    /// Registers a new FrameBuffer with the GraphicsSystem.
    /// @param framebuffer      New FrameBuffer to register.
    /// @throws NotUniqueError  If another FrameBuffer with the same ID already exists.
    void _register_new(FrameBufferPtr framebuffer);

    /// Registers a new Program with the GraphicsSystem.
    /// @param program          New Program to register.
    /// @throws NotUniqueError  If another Program with the same ID already exists.
    void _register_new(ShaderProgramPtr program);

    /// Call this function after the last shader has been compiled.
    /// Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
    /// guaranteed to do so.
    /// If you compile a new shader after calling this function, the driver will reallocate the compiler.
    void release_shader_compiler();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The FontManager.
    FontManagerPtr m_font_manager;

    // resources --------------------------------------------------------------

    /// All Textures managed by the GraphicsSystem.
    /// Note that the GraphicsSystem doesn't "own" the textures, they are shared pointers, but the GraphicsSystem
    /// deallocates all Textures when it is deleted.
    std::unordered_map<TextureId, TextureWeakPtr> m_textures;

    /// All Shaders managed by the GraphicsSystem.
    /// See `m_textures` for details on management.
    std::unordered_map<ShaderId, ShaderWeakPtr> m_shaders;

    /// All FrameBuffers managed by the GraphicsSystem.
    /// See `m_textures` for details on management.
    std::unordered_map<FrameBufferId, FrameBufferWeakPtr> m_framebuffers;

    /// All ShaderPrograms managed by the GraphicsSystem.
    /// See `m_textures` for details on management.
    std::unordered_map<ShaderProgramId, ShaderProgramWeakPtr> m_programs;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<TheGraphicsSystem, TheApplication> {
    friend TheApplication;

    /// Initializes the GraphicsSystem.
    static TheGraphicsSystem& initialize(valid_ptr<GLFWwindow*> shared_window) {
        TheGraphicsSystem& graphics_system = TheGraphicsSystem::_instance(raw_pointer(shared_window));
        graphics_system._post_initialization();
        return graphics_system;
    }

    /// Shuts the GraphicsSystem down for good.
    static void shutdown() { TheGraphicsSystem::get()._shutdown(); }
};

template<>
class Accessor<TheGraphicsSystem, Texture> {
    friend Texture;

    /// Registers a new Texture.
    /// @param texture          New Texture to register.
    /// @throws internal_error  If another Texture with the same ID already exists.
    static void register_new(TexturePtr texture) { TheGraphicsSystem::get()._register_new(std::move(texture)); }
};

template<>
class Accessor<TheGraphicsSystem, Shader> {
    friend Shader;

    /// Registers a new Shader.
    /// @param shader           New Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static void register_new(ShaderPtr shader) { TheGraphicsSystem::get()._register_new(std::move(shader)); }
};

template<>
class Accessor<TheGraphicsSystem, FrameBuffer> {
    friend FrameBuffer;

    /// Registers a new FrameBuffer.
    /// @param framebuffer      New FrameBuffer to register.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    static void register_new(FrameBufferPtr fbuffer) { TheGraphicsSystem::get()._register_new(std::move(fbuffer)); }
};

template<>
class Accessor<TheGraphicsSystem, ShaderProgram> {
    friend ShaderProgram;

    /// Registers a new Program.
    /// @param program          New Program to register.
    /// @throws internal_error  If another Program with the same ID already exists.
    static void register_new(ShaderProgramPtr program) { TheGraphicsSystem::get()._register_new(std::move(program)); }
};

NOTF_CLOSE_NAMESPACE
