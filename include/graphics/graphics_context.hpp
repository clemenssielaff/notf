#pragma once

#include <memory>
#include <vector>

#include "graphics/blend_mode.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/stencil_func.hpp"

namespace notf {

class Shader;
class Texture2;
class Window;

/**********************************************************************************************************************/

struct GraphicsContextArguments {

    /** Flag indicating whether the GraphicsContext will provide geometric antialiasing for its 2D shapes or not.
     * In a purely 2D application, this flag should be set to `true` since geometric antialiasing is cheaper than
     * full blown multisampling and looks just as good.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we don't
     * need the redundant geometrical antialiasing on top.
     */
    bool geometric_aa = true;

    /** When painting a semi-transparent stroke with enabled geometric AA, this will discard fragments around the
     * antialised edge of the stroke that would interfere with a self-crossing stroke.
     * This is a potentially expensive operation which is only noticeable in special circumstances, hence the flag.
     * TL;DR:
     *  - If you don't use geometric antialiasing, this has no effect.
     *  - If you do use it and notice no performance hit just leave it on.
     *  - If you do use it and it is slow and yet you see no artefacts, chances are you are not hitting the edge-case
     *    in question and you could disable this flag.
     * To see the effect in question, set this to false and play the test case with the rotating strokes.
     */
    bool save_alpha_stroke = true;

    /** Pixel ratio of the GraphicsContext.
     * Calculate the pixel ratio using `Window::get_buffer_size().width() / Window::get_window_size().width()`.
     * 1.0 means square pixels.
     */
    float pixel_ratio = 1.f;
};

/**********************************************************************************************************************/

/** The GraphicsContext.
 *
 * An Application has zero, one or multiple Windows.
 * Each Window has a RenderManager that takes care of the high-level Widget rendering.
 * Each RenderManager has a GraphicsContext (or maybe it is shared between Windows ... TBD)
 * The GraphicsContext is a wrapper around the OpenGL context and.
 */
class GraphicsContext {

    friend class Shader;
    friend class Texture2;

public: // methods
    /******************************************************************************************************************/
    /** Constructor. */
    GraphicsContext(const Window* window, const GraphicsContextArguments args);

    /** Destructor. */
    ~GraphicsContext();

    /** Makes the OpenGL context of this GraphicsContext current. */
    void make_current();

    /** The arguments with which the GraphicsContext was initialized. */
    const GraphicsContextArguments& get_args() const { return m_args; }

    /** Applies a new StencilFunction. */
    void set_stencil_func(const StencilFunc func);

    /** Applies the given stencil mask.  */
    void set_stencil_mask(const GLuint mask);

    /** Applies the given blend mode. */
    void set_blend_mode(const BlendMode mode);

    // TODO: pass a Context as argument to Texture and Shader instead / the GraphicsContext interface should be used for per-frame info

    /** Loads and returns a new Texture.
     * The result is empty if the Texture could not be loaded.
     */
    std::shared_ptr<Texture2> load_texture(const std::string& file_path);

    /** Builds a new OpenGL ES Shader from sources.
     * @param shader_name       Name of this Shader.
     * @param vertex_shader     Vertex shader source.
     * @param fragment_shader   Fragment shader source.
     * @return                  Shader instance, is empty if the compilation failed.
     */
    std::shared_ptr<Shader> build_shader(const std::string& name,
                                         const std::string& vertex_shader_source,
                                         const std::string& fragment_shader_source);

    /** Resets the cached state, forcing the GraphicsContext to explicitly re-bind and re-set all values. */
    void force_reloads();

private: // methods for friends
    /** Binds the Texture with the given ID, but only if it is not the currently bound one. */
    void _bind_texture(const GLuint texture_id);

    /** Binds the Shader with the given ID, but only if it is not the currently bound one. */
    void _bind_shader(const GLuint shader_id);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** Argument struct to initialize the GraphicsContext. */
    GraphicsContextArguments m_args;

    /* OpenGL state cache *********************************************************************************************/

    /** Cached stencil function to avoid unnecessary rebindings. */
    StencilFunc m_stencil_func;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /* Cached blend mode to avoid unnecessary rebindings. */
    BlendMode m_blend_mode;

    /* Textures *******************************************************************************************************/

    /** The ID of the currently bound Texture in order to avoid unnecessary rebindings. */
    GLuint m_bound_texture;

    /** All Textures managed by this Context.
     * Note that the Context doesn't "own" the textures, they are shared pointers, but the Render Context deallocates
     * all Textures when it is deleted.
     */
    std::vector<std::weak_ptr<Texture2>> m_textures;

    /* Shaders ********************************************************************************************************/

    /** The ID of the currently bound Shader in order to avoid unnecessary rebindings. */
    GLuint m_bound_shader;

    /** All Shaders managed by this Context.
     * See `m_textures` for details on management.
     */
    std::vector<std::weak_ptr<Shader>> m_shaders;

private: // static fields
    /** The current GraphicsContext. */
    static GraphicsContext* s_current_context;
};

} // namespace notf
