#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "./gl_forwards.hpp"
#include "common/color.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"
#include "common/size2.hpp"

struct GLFWwindow;

namespace notf {

//====================================================================================================================//

// forward declaration of relevant ID types
using FrameBufferId = IdType<FrameBuffer, GLuint>;
using ShaderId      = IdType<Shader, GLuint>;
using TextureId     = IdType<Texture, GLuint>;

//====================================================================================================================//

/// HTML5 canvas-like approach to blending the results of multiple OpenGL drawings.
/// Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
/// The source image is the image being rendered, and the destination image the current state of the bitmap.
struct BlendMode {

    // types ---------------------------------------------------------------------------------------------------------//
    /// Blend mode, can be set for RGB and the alpha channel separately.
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
    /// Blend mode for colors.
    Mode rgb;

    /// Blend mode for transparency.
    Mode alpha;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default constructor.
    BlendMode() : rgb(DEFAULT), alpha(DEFAULT) {}

    /// Value constructor.
    /// @param mode Single blend mode for both rgb and alpha.
    BlendMode(const Mode mode) : rgb(mode), alpha(mode) {}

    /// Separate blend modes for both rgb and alpha.
    /// @param color    RGB blend mode.
    /// @param alpha    Alpha blend mode.
    BlendMode(const Mode color, const Mode alpha) : rgb(color), alpha(alpha) {}

    /// Equality operator.
    /// @param other    Blend mode to test against.
    bool operator==(const BlendMode& other) const { return rgb == other.rgb && alpha == other.alpha; }

    /// Inequality operator.
    /// @param other    Blend mode to test against.
    bool operator!=(const BlendMode& other) const { return rgb != other.rgb || alpha != other.alpha; }
};

//====================================================================================================================//

/// Direction to cull in the culling test.
enum CullFace : unsigned char {
    BACK,  ///< Do not render back-facing faces (default).
    FRONT, ///< Do not render front-facing faces.
    BOTH,  ///< Cull all faces.
    NONE,  ///< Render bot front- and back-facing faces.
    DEFAULT = BACK,
};

//====================================================================================================================//

/// The GraphicsContext is an abstraction of the OpenGL graphics context.
/// It is the object owning all NoTF client objects like shaders and textures.
class GraphicsContext {

    // managed classes have access to the GraphicsContext's internals to register themselves
    friend class Shader;
    friend class Texture;
    friend class FrameBuffer;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Helper struct that can be used to test whether selected extensions are available in the OpenGL ES driver.
    /// Only tests for extensions on first instantiation.
    struct Extensions {

        /// Is anisotropic filtering of textures supported?
        bool anisotropic_filter;

        /// Does the GPU support nVidia GPU shader5 extensions.
        /// @see   https://www.khronos.org/registry/OpenGL/extensions/NV/NV_gpu_shader5.txt
        bool nv_gpu_shader5;

    private:
        friend class GraphicsContext;

        /// Default constructor.
        Extensions();
    };

    /// Helper struct containing variables that need to be read from OpenGL at runtime and won't change over the
    /// course of the app.
    struct Environment {

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

    private:
        friend class GraphicsContext;

        /// Default constructor.
        Environment();
    };

    enum Buffer : unsigned char { COLOR = 1u << 1, DEPTH = 1u << 2, STENCIL = 1u << 3 };
    using BufferFlags = std::underlying_type<Buffer>::type;

private:
    /// Graphics state.
    struct State {

        BlendMode blend_mode = BlendMode::DEFAULT;

        CullFace cull_face = CullFace::DEFAULT;

        GLuint stencil_mask = 0xffffffff;

        std::vector<TextureConstPtr> texture_slots = {};

        PipelinePtr pipeline = {};

        FrameBufferPtr framebuffer = {};

        /// Color applied when the bound framebuffer is cleared.
        Color clear_color = Color::black();

        /// Render size in pixels.
        Size2i render_size;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(GraphicsContext)

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
    static const Extensions& extensions();

    /// Creates and initializes information about the graphics environment.
    static const Environment& environment();

    /// The current state of the context.
    const State& state() const { return m_state; }

    /// Returns the size of the context's window in pixels.
    /// Note that this might not be the current render size. For example, if you are currently rendering into a
    /// FrameBuffer that has a color texture of size 128x128, the window size is most likely much larger than that.
    Size2i window_size() const;

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

    /// Updates the size of the OpenGL viewport.
    /// This changes the size of the 2D coordinate system that is rendered into.
    /// @param buffer_size  New render size.
    /// @param force        Ignore the current state and always make the OpenGL call.
    void set_render_size(Size2i buffer_size, const bool force = false);

    /// Sets the new clear color.
    /// @param color    Color to apply.
    /// @param buffers  Buffers to clear.
    /// @param force    Ignore the current state and always make the OpenGL call.
    void clear(Color color, const BufferFlags buffers = Buffer::COLOR, const bool force = false);

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
    TexturePtr texture(const TextureId& id) const;

    /// Binds the given texture at the given texture slot.
    /// Only results in an OpenGL call if the texture is not currently bound.
    /// @param texture          Texture to bind.
    /// @param slot             Texture slot to bind the texture to.
    /// @throws runtime_error   If the texture is not valid.
    /// @throws runtime_error   If slot is >= than the number of texture slots in the environment.
    void bind_texture(const Texture* texture, uint slot);
    void bind_texture(const TexturePtr& texture, uint slot) { bind_texture(texture.get(), slot); }

    /// Unbinds the current texture and clears the context's texture stack.
    /// @param slot             Texture slot to clear.
    /// @throws runtime_error   If slot is >= than the number of texture slots in the environment.
    void unbind_texture(uint slot);

    /// Unbinds bound texures from all slots.
    void unbind_all_textures();

    // shader -----------------------------------------------------------------

    /// Checks whether this context contains a Shader with the given ID.
    /// @param id   ID of the Shader.
    bool has_shader(const ShaderId& id) { return m_shaders.count(id) != 0; }

    /// Finds and returns a Shader of this context by its ID.
    /// @param id               ID of the Shader.
    /// @throws out_of_range    If the context does not contain a Shader with the given id.
    ShaderPtr shader(const ShaderId& id) const;

    // pipeline ---------------------------------------------------------------

    /// Binds the given Pipeline, if it is not already bound.
    /// @param pipeline Pipeline to bind.
    void bind_pipeline(const PipelinePtr& pipeline);

    /// Unbinds the current Pipeline.
    void unbind_pipeline();

    // frambuffer -------------------------------------------------------------

    /// Checks whether this context contains a FrameBuffer with the given ID.
    /// @param id   ID of the FrameBuffer.
    bool has_framebuffer(const FrameBufferId& id) { return m_framebuffers.count(id) != 0; }

    /// Finds and returns a FrameBuffer of this context by its ID.
    /// @param id               ID of the FrameBuffer.
    /// @throws out_of_range    If the context does not contain a FrameBuffer with the given id.
    FrameBufferPtr framebuffer(const FrameBufferId& id) const;

    /// Binds the given FrameBuffer, if it is not already bound.
    /// @param framebuffer  FrameBuffer to bind.
    void bind_framebuffer(const FrameBufferPtr& framebuffer);

    /// Unbinds the current FrameBuffer.
    void unbind_framebuffer();

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Create a new State.
    State _create_state() const;

    /// Registers a new Texture with this GraphicsContext.
    /// @param texture          New Texture to register.
    /// @throws internal_error  If another Texture with the same ID already exists.
    void register_new(TexturePtr texture);

    /// Registers a new Shader with this GraphicsContext.
    /// @param texture          New Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    void register_new(ShaderPtr shader);

    /// Registers a new FrameBuffer with this GraphicsContext.
    /// @param texture          New FrameBuffer to register.
    /// @throws internal_error  If another FrameBuffer with the same ID already exists.
    void register_new(FrameBufferPtr framebuffer);

    /// Call this function after the last shader has been compiled.
    /// Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
    /// guaranteed to do so.
    /// If you compile a new shader after calling this function, the driver will reallocate the compiler.
    void release_shader_compiler();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The GLFW window displaying the contents of this context.
    GLFWwindow* m_window;

    /// The current state of the context.
    State m_state;

    /// True if this context has vsync enabled.
    bool m_has_vsync;

    /// All Textures managed by this Context.
    /// Note that the Context doesn't "own" the textures, they are shared pointers, but the Render Context deallocates
    /// all Textures when it is deleted.
    std::unordered_map<TextureId, std::weak_ptr<Texture>> m_textures;

    /// All Shaders managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<ShaderId, std::weak_ptr<Shader>> m_shaders;

    /// All FrameBuffers managed by this Context.
    /// See `m_textures` for details on management.
    std::unordered_map<FrameBufferId, std::weak_ptr<FrameBuffer>> m_framebuffers;
};

} // namespace notf
