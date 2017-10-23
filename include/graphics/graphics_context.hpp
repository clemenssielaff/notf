#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "common/meta.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/stencil_func.hpp"

class GLFWwindow;

namespace notf {

class FontManager;

DEFINE_SHARED_POINTERS(class, Shader);
DEFINE_SHARED_POINTERS(class, Texture2);

//*********************************************************************************************************************/
//*********************************************************************************************************************/

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

    /** Makes the OpenGL context of this GraphicsContext current. */
    void make_current();

    /** Checks whether this graphics context is current on the calling thread. */
    bool is_current() const;

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

    /** Applies the given blend mode.
     * @throws std::runtime_error   If the graphics context is not current.
     */
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


private: // static fields
    /** The current GraphicsContext. */
    static GraphicsContext* s_current_context;
};

} // namespace notf
