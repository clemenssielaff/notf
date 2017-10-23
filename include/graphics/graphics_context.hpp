#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "common/meta.hpp"
#include "graphics/gl_forwards.hpp"

class GLFWwindow;

namespace notf {

DEFINE_SHARED_POINTERS(class, Shader);
DEFINE_SHARED_POINTERS(class, Texture2);

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
    BlendMode()
        : rgb(DEFAULT)
        , alpha(DEFAULT) {}

    /// @brief Value constructor.
    /// @param mode Single blend mode for both rgb and alpha.
    BlendMode(const Mode mode)
        : rgb(mode)
        , alpha(mode) {}

    /// @brief Separate blend modes for both rgb and alpha.
    /// @param color    RGB blend mode.
    /// @param alpha    Alpha blend mode.
    BlendMode(const Mode color, const Mode alpha)
        : rgb(color)
        , alpha(alpha) {}

    /// @brief Equality operator.
    /// @param other    Blend mode to test against.
    bool operator==(const BlendMode& other) const { return rgb == other.rgb && alpha == other.alpha; }

    /// @brief Inequality operator.
    /// @param other    Blend mode to test against.
    bool operator!=(const BlendMode& other) const { return rgb != other.rgb || alpha != other.alpha; }
};

//====================================================================================================================//

struct StencilFunc {

    // types ---------------------------------------------------------------------------------------------------------//
    /// @brief Blend mode, can be set for RGB and the alpha channel separately.
    enum Stencil : unsigned char {
        ALWAYS,   ///< Always passes.
        NEVER,    ///< Always fails.
        LESS,     ///< Passes if ( ref & mask ) < ( stencil & mask ).
        LEQUAL,   ///< Passes if ( ref & mask ) <= ( stencil & mask ).
        GREATER,  ///< Passes if ( ref & mask ) > ( stencil & mask ).
        GEQUAL,   ///< Passes if ( ref & mask ) >= ( stencil & mask ).
        EQUAL,    ///< Passes if ( ref & mask ) = ( stencil & mask ).
        NOTEQUAL, ///< Passes if ( ref & mask ) != ( stencil & mask ).
        DEFAULT = ALWAYS,
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief The Stencil function.
    Stencil function;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    StencilFunc()
        : function(ALWAYS) {}

    /// @brief Value Constructor.
    /// @param function Stencil function.
    StencilFunc(const Stencil function)
        : function(function) {}

    /// @brief Equality operator.
    /// @param other    Stencil function to test against.
    bool operator==(const StencilFunc& other) const { return function == other.function; }

    /// @brief Inequality operator.
    /// @param other    Stencil function to test against.
    bool operator!=(const StencilFunc& other) const { return function != other.function; }
};

//====================================================================================================================//

/** The GraphicsContext is an abstraction of the OpenGL graphics context.
 * It is the object owning all NoTF client objects like shaders and textures.
 */
class GraphicsContext {

    friend class Shader;
    friend class Texture2;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /** Helper struct that can be used to test whether selected extensions are available in the OpenGL ES driver.
     * Only tests for extensions on first instantiation.
     */
    struct GLExtensions {

        friend class GraphicsContext;

        /** Is anisotropic filtering of textures supported? */
        bool anisotropic_filter;

    private: // methods ***************************************************************************************************/
        /** Constructor. */
        GLExtensions();
    };

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(GraphicsContext)

    /** Constructor.
     * @param window                GLFW window displaying the contents of this context.
     * @throws std::runtime_error   If the given window is invalid.
     * @throws std::runtime_error   If another current OpenGL context exits.
     */
    GraphicsContext(GLFWwindow* window);

    /** Destructor. */
    ~GraphicsContext();

    /// @brief Creates and returns an GLExtension instance.
    const GLExtensions& extensions() const
    {
        static const GLExtensions singleton;
        return singleton;
    }

    /** En- or disables vsync (enabled by default).
     * @throws std::runtime_error   If the graphics context is not current.
     */
    void set_vsync(const bool enabled);

    /** Applies a new StencilFunction.
     * @throws std::runtime_error   If the graphics context is not current.
     */
    void set_stencil_func(const StencilFunc func);

    /** Applies the given stencil mask.
     * @throws std::runtime_error   If the graphics context is not current.
     */
    void set_stencil_mask(const GLuint mask);

    /// @brief Applies the given blend mode to OpenGL.
    /// @brief mode Blend mode to apply.
    /// @throws std::runtime_error   If the graphics context is not current.
    void set_blend_mode(const BlendMode mode);

    /** Binds the given texture.
     * Only results in an OpenGL call if the texture is not currently bound.
     * @param   texture             Texture to bind.
     * @throws std::runtime_error   If the texture is not valid.
     * @throws std::runtime_error   If the graphics context is not current.
     */
    void push_texture(Texture2Ptr texture);

    /** Re-binds the last bound texture.
     * @throws  std::runtime_error  If the graphics context is not current.
     */
    void pop_texture();

    /** Unbinds the current texture and clears the context's texture stack.
     * @throws  std::runtime_error  If the graphics context is not current.
     */
    void clear_texture();

    /** Binds the given Shader.
     * Only results in an OpenGL call if the shader is not currently bound.
     * @param shader                Shader to bind.
     * @throws std::runtime_error   If the shader is not valid.
     * @throws std::runtime_error   If the graphics context is not current.
     */
    void push_shader(ShaderPtr shader);

    /** Re-binds the last bound shader.
     * @throws  std::runtime_error  If the graphics context is not current.
     */
    void pop_shader();

    /** Unbinds the current shader and clears the context's shader stack.
     * @throws  std::runtime_error  If the graphics context is not current.
     */
    void clear_shader();

    /** Call this function after the last shader has been compiled.
     * Might cause the driver to release the resources allocated for the compiler to free up some space, but is not
     * guaranteed to do so.
     * If you compile a new shader after calling this function, the driver will reallocate the compiler.
     */
    void release_shader_compiler();

private: // fields ****************************************************************************************************/
    /** The GLFW window displaying the contents of this context. */
    GLFWwindow* m_window;

    /** Id of the thread on which the context is current. */
    std::thread::id m_current_thread;

    /** True if this context has vsync enabled. */
    bool m_has_vsync;

    /** Cached stencil function to avoid unnecessary rebindings. */
    StencilFunc m_stencil_func;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /* Cached blend mode to avoid unnecessary rebindings. */
    BlendMode m_blend_mode;

    /** Stack with the currently bound Texture on top. */
    std::vector<Texture2Ptr> m_texture_stack;

    /** All Textures managed by this Context.
     * Note that the Context doesn't "own" the textures, they are shared pointers, but the Render Context deallocates
     * all Textures when it is deleted.
     */
    std::vector<std::weak_ptr<Texture2>> m_textures;

    /** Stack with the currently bound Shader on top. */
    std::vector<ShaderPtr> m_shader_stack;

    /** All Shaders managed by this Context.
     * See `m_textures` for details on management.
     */
    std::vector<std::weak_ptr<Shader>> m_shaders;
};

} // namespace notf
