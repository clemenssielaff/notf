#pragma once

#include <memory>
#include <vector>

#include "graphics/blend_mode.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/stencil_func.hpp"

namespace notf {

class FontManager;
class Shader;
class Texture2;
class Window;

/**********************************************************************************************************************/

struct GraphicsContextOptions {

    /** Flag indicating whether the GraphicsContext will provide geometric antialiasing for its 2D shapes or not.
     * In a purely 2D application, this flag should be set to `true` since geometric antialiasing is cheaper than
     * full blown multisampling and looks just as good.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we might not
     * need the redundant geometrical antialiasing on top.
     */
    bool geometric_aa = true;

    /** When drawing transparent strokes, this flag will make sure that the stroke has a consistent alpha.
     * It does so by creating two stroke calls - one for the stencil and one for the actual fill.
     * This is expensive and becomes even more so, because the fragment shader will have to discard many fragments,
     * which might cause a massive slowdown on some machines (cuts fps in half on mine).
     * Since the effect is not visible if you don't draw thick, transparent strokes, this is off by default.
     * If you do see areas in your stroke that are darker than others you might want to enable it.
     * Also, if you try it out and it does not cause a massive performance hit, you might just as well leave it on
     * because it is the Right Way™ to draw strokes.
     */
    bool stencil_strokes = true;

    /** Pixel ratio of the GraphicsContext.
     * Calculate the pixel ratio using `Window::get_buffer_size().width() / Window::get_window_size().width()`.
     * 1.0 means square pixels.
     */
    float pixel_ratio = 1.f;

    /** Limit of the ration of a joint's miter length to its stroke width. */
    float miter_limit = 2.4f;
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
    GraphicsContext(const Window* window, const GraphicsContextOptions args);

    /** Destructor. */
    ~GraphicsContext();

    /** Makes the OpenGL context of this GraphicsContext current. */
    void make_current();

    /** The curent options of the GraphicsContext. */
    const GraphicsContextOptions& get_options() const { return m_options; }

    /** The Font Manager. */
    FontManager& get_font_manager() const { return *m_font_manager.get(); }

    /** Applies a new StencilFunction. */
    void set_stencil_func(const StencilFunc func);

    /** Applies the given stencil mask.  */
    void set_stencil_mask(const GLuint mask);

    /** Applies the given blend mode. */
    void set_blend_mode(const BlendMode mode);

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
    GraphicsContextOptions m_options;

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

    /* Dedicated context managers (at the end so the rest is already initialized when they are created) ***************/

    /** The Font Manager. */
    std::unique_ptr<FontManager> m_font_manager;

private: // static fields
    /** The current GraphicsContext. */
    static GraphicsContext* s_current_context;
};

} // namespace notf
