#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "./gl_forwards.hpp"
#include "common/forwards.hpp"

struct GLFWwindow;

namespace notf {

//====================================================================================================================//

/// @brief HTML5 canvas-like approach to blending the results of multiple OpenGL drawings.
/// Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
/// The source image is the image being rendered, and the destination image the current state of the bitmap.
struct BlendMode {

    // types ---------------------------------------------------------------------------------------------------------//
    /// @brief Blend mode, can be set for RGB and the alpha channel separately.
    enum Mode : unsigned char {
        SOURCE_OVER,      ///< Display the source image wherever the source image is opaque, the destination image
                          /// elsewhere (default).
        SOURCE_IN,        ///< Display the source image where the both are opaque, transparency elsewhere.
        SOURCE_OUT,       ///< Display the source image where the the source is opaque and the destination transparent,
                          ///  transparency elsewhere.
        SOURCE_ATOP,      ///< Source image wherever both images are opaque.
                          ///  Display the destination image wherever the destination image is opaque but the source
                          ///  image is transparent. Display transparency elsewhere.
        DESTINATION_OVER, ///< Same as SOURCE_OVER with the destination instead of the source.
        DESTINATION_IN,   ///< Same as SOURCE_IN with the destination instead of the source.
        DESTINATION_OUT,  ///< Same as SOURCE_OUT with the destination instead of the source.
        DESTINATION_ATOP, ///< Same as SOURCE_ATOP with the destination instead of the source.
        LIGHTER,          ///< The sum of the source image and destination image, with 255 (100%) as a limit.
        COPY,             ///< Source image instead of the destination image (overwrite destination).
        XOR,              ///< Exclusive OR of the source image and destination image.
        DEFAULT = SOURCE_OVER,
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Blend mode for colors.
    Mode rgb;

    /// @brief Blend mode for transparency.
    Mode alpha;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    BlendMode() : rgb(DEFAULT), alpha(DEFAULT) {}

    /// @brief Value constructor.
    /// @param mode Single blend mode for both rgb and alpha.
    BlendMode(const Mode mode) : rgb(mode), alpha(mode) {}

    /// @brief Separate blend modes for both rgb and alpha.
    /// @param color    RGB blend mode.
    /// @param alpha    Alpha blend mode.
    BlendMode(const Mode color, const Mode alpha) : rgb(color), alpha(alpha) {}

    /// @brief Equality operator.
    /// @param other    Blend mode to test against.
    bool operator==(const BlendMode& other) const { return rgb == other.rgb && alpha == other.alpha; }

    /// @brief Inequality operator.
    /// @param other    Blend mode to test against.
    bool operator!=(const BlendMode& other) const { return rgb != other.rgb || alpha != other.alpha; }
};

//====================================================================================================================//

/// @brief Direction to cull in the culling test.
enum CullFace : unsigned char {
    BACK,  ///< Do not render back-facing faces (default).
    FRONT, ///< Do not render front-facing faces.
    BOTH,  ///< Cull all faces.
    NONE,  ///< Render bot front- and back-facing faces.
    DEFAULT = BACK,
};

//====================================================================================================================//

/// @brief The GraphicsContext is an abstraction of the OpenGL graphics context.
/// It is the object owning all NoTF client objects like shaders and textures.
class GraphicsContext {

    friend class Shader;
    friend class Texture;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Helper struct that can be used to test whether selected extensions are available in the OpenGL ES driver.
    /// Only tests for extensions on first instantiation.
    struct Extensions {

        /// @brief Is anisotropic filtering of textures supported?
        bool anisotropic_filter;

    private:
        friend class GraphicsContext;

        /// @brief Default constructor.
        Extensions();
    };

    /// @brief Helper struct containing variables that need to be read from OpenGL at runtime and won't change over the
    /// course of the app.
    struct Environment {

        /// @brief Number of texture slots.
        GLuint texture_slot_count;

        /// @brief Maximum height and width of a render buffer in pixels.
        GLuint max_render_buffer_size;

        /// @brief Number of available color attachments for a frame buffer.
        GLuint color_attachment_count;

    private:
        friend class GraphicsContext;

        /// @brief Default constructor.
        Environment();
    };

private:
    /// @brief Graphics state
    struct State {

        BlendMode blend_mode;

        CullFace cull_face = CullFace::DEFAULT;

        bool enable_depth = false;

        bool enable_dithering = true;

        float polygon_offset = 0;

        bool enable_discard = false;

        bool enable_scissor = false;

        GLuint stencil_mask = 0xffffffff;

        std::vector<TexturePtr> texture_slots = {};

        PipelinePtr pipeline;

        FrameBufferPtr framebuffer;

    }; // TODO: a lot of the variables in Graphics::State could be combined into a bitset

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(GraphicsContext)

    /// Constructor.
    /// @param window                GLFW window displaying the contents of this context.
    /// @throws std::runtime_error   If the given window is invalid.
    /// @throws std::runtime_error   If another current OpenGL context exits.
    GraphicsContext(GLFWwindow* window);

    /// Destructor.
    ~GraphicsContext();

    /// @brief Tests whether two Graphics Contexts are the same.
    bool operator==(const GraphicsContext& other) const { return m_window == other.m_window; }

    /// @brief Creates and returns an GLExtension instance.
    static const Extensions& extensions();

    /// @brief Creates and initializes information about the graphics environment.
    static const Environment& environment();

    /// @brief Create a new State;
    State create_state() const;

    /// @brief En- or disables vsync (enabled by default).
    /// @param enabled  Whether to enable or disable vsync.
    void set_vsync(const bool enabled);

    /// @brief Applies the given stencil mask.
    void set_stencil_mask(const GLuint mask);

    /// @brief Applies the given blend mode to OpenGL.
    /// @brief mode Blend mode to apply.
    void set_blend_mode(const BlendMode mode);

    // texture ----------------------------------------------------------------

    /// @brief Checks whether this context contains a Texture with the given name.
    /// @param name Name of the Texture.
    bool has_texture(const std::string& name) { return m_shaders.count(name) != 0; }

    /// @brief Finds and returns a Texture of this context by its name.
    /// @param name                 Name of the Texture.
    /// @throws std::out_of_range   If the context does not contain a Texture with the given name.
    TexturePtr texture(const std::string& name) const { return m_textures.at(name).lock(); }

    /// @brief Binds the given texture at the given texture slot.
    /// Only results in an OpenGL call if the texture is not currently bound.
    /// @param texture              Texture to bind.
    /// @param slot                 Texture slot to bind the texture to.
    /// @throws std::runtime_error  If the texture is not valid.
    /// @throws std::runtime_error  If slot is >= than the number of texture slots in the environment.
    void bind_texture(Texture* texture, uint slot);
    void bind_texture(TexturePtr& texture, uint slot) { bind_texture(texture.get(), slot); }

    /// @brief Unbinds the current texture and clears the context's texture stack.
    /// @param slot                 Texture slot to clear.
    /// @throws std::runtime_error  If slot is >= than the number of texture slots in the environment.
    void unbind_texture(uint slot);

    /// @brief Unbinds bound texures from all slots.
    void unbind_all_textures();

    // shader -----------------------------------------------------------------

    /// @brief Checks whether this context contains a Shader with the given name.
    /// @param name Name of the Shader.
    bool has_shader(const std::string& name) { return m_shaders.count(name) != 0; }

    /// @brief Finds and returns a Shader of this context by its name.
    /// @param name                 Name of the Shader.
    /// @throws std::out_of_range   If the context does not contain a Shader with the given name.
    ShaderPtr shader(const std::string& name) const { return m_shaders.at(name).lock(); }

    // pipeline ---------------------------------------------------------------

    /// @brief Binds the given Pipeline, if it is not already bound.
    /// @param pipeline Pipeline to bind.
    void bind_pipeline(PipelinePtr& pipeline);

    /// @brief Unbinds the current Pipeline.
    void unbind_pipeline();

    // frambuffer -------------------------------------------------------------

    /// @brief Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    void bind_framebuffer(FrameBufferPtr& framebuffer);

    /// @brief Unbinds the current FrameBuffer.
    void unbind_framebuffer();

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// @brief Call this function after the last shader has been compiled.
    /// Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
    /// guaranteed to do so.
    /// If you compile a new shader after calling th6is function, the driver will reallocate the compiler.
    void _release_shader_compiler();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief The GLFW window displaying the contents of this context.
    GLFWwindow* m_window;

    /// @brief The current state of the context.
    State m_state;

    /// @brief True if this context has vsync enabled.
    bool m_has_vsync;

    /// @brief All Textures managed by this Context.
    /// Note that the Context doesn't "own" the textures, they are shared pointers, but the Render Context deallocates
    /// all Textures when it is deleted.
    std::unordered_map<std::string, std::weak_ptr<Texture>> m_textures;

    /// @brief All Shaders managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<std::string, std::weak_ptr<Shader>> m_shaders;
};

} // namespace notf
