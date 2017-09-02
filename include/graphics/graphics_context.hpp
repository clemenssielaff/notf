#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "common/meta.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/stencil_func.hpp"

namespace notf {

class FontManager;
class Shader;
class Texture2;
class Window;

/**********************************************************************************************************************/

/** The GraphicsContext is an abstraction of the OpenGL graphics context.
 * It is the object owning all NoTF client objects like shaders and textures.
 */
class GraphicsContext {

    friend class Shader;
    friend class Texture2;

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(GraphicsContext)

    /** Constructor.
     * @throws  std::runtime_error If the given window is invalid.
     * @throws  std::runtime_error If another current OpenGL context exits.
     * @throws  std::runtime_error If the given window does not have a valid and current OpenGL context.
     */
    GraphicsContext(const Window* window);

    /** Destructor. */
    ~GraphicsContext();

    /** Makes the OpenGL context of this GraphicsContext current. */
    void make_current();

    /** Checks whether this graphics context is current on the calling thread. */
    bool is_current() const;

    /** The Font Manager. */
    FontManager& get_font_manager() const { return *m_font_manager.get(); }

    /** Applies a new StencilFunction. */
    void set_stencil_func(const StencilFunc func);

    /** Applies the given stencil mask.  */
    void set_stencil_mask(const GLuint mask);

    /** Applies the given blend mode. */
    void set_blend_mode(const BlendMode mode);

    /** Binds the given texture.
     * @param   texture     Texture to bind.
     * @throws              std::runtime_error if the graphics context is not current.
     */
    void bind_texture(Texture2* texture);

    /** Binds the Shader with the given ID.
     * @throws  std::runtime_error if the graphics context is not current.
     */
    void bind_shader(Shader* shader);

    /** Resets the cached state, forcing the GraphicsContext to explicitly re-bind and re-set all values. */
    void force_reloads();

private: // fields ****************************************************************************************************/
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** Id of the thread on which the context is current. */
    std::thread::id m_current_thread;

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
