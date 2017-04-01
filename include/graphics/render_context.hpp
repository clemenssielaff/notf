#pragma once

#include <memory>
#include <vector>

#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/stencil_func.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class Window;
struct Scissor;
class Shader;
class Texture2;

/**********************************************************************************************************************/

struct RenderContextArguments {

    /** Flag indicating whether the RenderContext will provide geometric antialiasing for its 2D shapes or not.
     * In a purely 2D application, this flag should be set to `true` since geometric antialiasing is cheaper than
     * full blown multisampling and looks just as good.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we don't
     * need the redundant geometrical antialiasing on top.
     */
    bool enable_geometric_aa = true;

    /** Pixel ratio of the RenderContext.
     * Calculate the pixel ratio using `Window::get_buffer_size().width() / Window::get_window_size().width()`.
     * 1.0 means square pixels.
     */
    float pixel_ratio = 1.f;
};

/**********************************************************************************************************************/

/** The RenderContext.
 *
 * An Application has zero, one or multiple Windows.
 * Each Window has a RenderManager that takes care of the high-level Widget rendering.
 * Each RenderManager has a RenderContext (or maybe it is shared between Windows ... TBD)
 * The RenderContext is a wrapper around the OpenGL context and.
 */
class RenderContext {

    friend class RenderManager;
    friend class Shader;
    friend class Texture2;
    friend class Widget;

private: // classes
    /******************************************************************************************************************/
    struct Path {
        GLint fillOffset    = 0;
        GLsizei fillCount   = 0;
        GLint strokeOffset  = 0;
        GLsizei strokeCount = 0;
    };

    /******************************************************************************************************************/

    struct Call {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
        };
        Type type;
        size_t pathOffset;
        size_t pathCount;
        GLintptr uniformOffset;
        std::shared_ptr<Texture2> texture;
        GLint triangleOffset;
    };

    /******************************************************************************************************************/

    struct ShaderVariables { //  TODO: replace more of the float[]s with explicit types
        enum class Type : GLint {
            GRADIENT,
            IMAGE,
            SIMPLE,
        };

        ShaderVariables() // we'll see if we need this initialization to zero at all
            : innerCol(),
              outerCol(),
              radius{0},
              feather{0},
              strokeMult{0},
              strokeThr{0},
              texType{0},
              type{Type::SIMPLE}
        {
            for (auto i = 0; i < 12; ++i) {
                scissorMat[i] = 0;
                paintMat[i]   = 0;
            }
            for (auto i = 0; i < 2; ++i) {
                scissorExt[i]   = 0;
                scissorScale[i] = 0;
                extent[i]       = 0;
            }
        }

        float scissorMat[12]; // matrices are actually 3 vec4s
        float paintMat[12];
        Color innerCol;
        Color outerCol;
        float scissorExt[2];
        float scissorScale[2];
        float extent[2];
        float radius;
        float feather;
        float strokeMult;
        float strokeThr;
        int texType;
        Type type;
    };

    friend void paint_to_frag(ShaderVariables& frag, const Paint& paint, const Scissor& scissor,
                              const float stroke_width, const float fringe, const float stroke_threshold);

    /******************************************************************************************************************/

    struct CellShader {
        /** The actual Cell Shader. */
        std::shared_ptr<Shader> shader;

        /** Location of the `view_size` uniform in the Shader. */
        GLint viewsize;

        /** Location of the `textures` uniform in the Shader. */
        GLint texture;

        /** Location of the `variables` uniform in the Shader. */
        GLuint variables;
    };

public: // methods
    /******************************************************************************************************************/
    /** Constructor. */
    RenderContext(const Window* window, const RenderContextArguments args);

    /** Destructor. */
    ~RenderContext();

    /** Makes the OpenGL context of this RenderContext current. */
    void make_current();

    /** Time at the beginning of the current frame. */
    Time get_time() const { return m_time; }

    /** The mouse position relative to the Window's top-left corner. */
    const Vector2f& get_mouse_pos() const { return m_mouse_pos; }

    /** The pixel ratio of the RenderContext. */
    float get_pixel_ratio() const { return m_args.pixel_ratio; }

    /** Whether Cells should provide their own geometric antialiasing or not. */
    bool provides_geometric_aa() const { return m_args.enable_geometric_aa; }

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float get_distance_tolerance() const { return 0.01f / m_args.pixel_ratio; }

    /** Tesselation density when creating rounded shapes. */
    float get_tesselation_tolerance() const { return 0.25f / m_args.pixel_ratio; }

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float get_fringe_width() const { return 1.f / m_args.pixel_ratio; }

    /** Applies a new StencilFunction. */
    void set_stencil_func(const StencilFunc func);

    /** Applies the given stencil mask.  */
    void set_stencil_mask(const GLuint mask);

    /** Applies the given blend mode. */
    void set_blend_mode(const BlendMode mode);

    // TODO: pass a Context as argument to Texture and Shader instead / the RenderContext interface should be used for per-frame info

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

private: // methods for friends
    /** Updates the time at the start of the rendering. */
    void _set_time(Time time) { m_time = time; }

    /** Updates the mouse position. */
    void _set_mouse_pos(Vector2f pos) { m_mouse_pos = std::move(pos); }

    /** Updates buffer size. */
    void _set_buffer_size(Size2f buffer) { m_buffer_size = std::move(buffer); }

    /** Begins a new frame. */
    void _begin_frame(const Size2i& buffer_size);

    /** Aborts the drawing of the current frame if something went wrong. */
    void _reset();

    /** Performs all stored Calls. */
    void _finish_frame();

    /** Adds render calls from a given Cell. */
    void _add_cell(const Cell& cell);

    /** Binds the Texture with the given ID, but only if it is not the currently bound one. */
    void _bind_texture(const GLuint texture_id);

    /** Binds the Shader with the given ID, but only if it is not the currently bound one. */
    void _bind_shader(const GLuint shader_id);

private: // methods
    /** Fills a simple, convex shape. */
    void _perform_convex_fill(const Call& call);

    /** Fills multiple or complex shapes in one call. */
    void _perform_fill(const Call& call);

    /** Strokes a path. */
    void _perform_stroke(const Call& call);

private: // static methods
    /** Size (in bytes) of a ShaderVariables struct. */
    static constexpr GLintptr fragmentSize() { return sizeof(ShaderVariables); }

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** Argument struct to initialize the RenderContext. */
    RenderContextArguments m_args;

    /** All Calls that were collected during during the frame. */
    std::vector<Call> m_calls;

    /** Indices of `m_vertices` of all Paths drawn during the frame. */
    std::vector<Path> m_paths;

    /** Vertices (global, not path specific?) */
    std::vector<Vertex> m_vertices;

    /** ShaderVariables for each Call. */
    std::vector<ShaderVariables> m_shader_variables;

    /* Per-frame infos ************************************************************************************************/

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f m_buffer_size;

    /** Time at the beginning of the current frame. */
    Time m_time;

    /** The mouse position relative to the Window's top-left corner. */
    Vector2f m_mouse_pos;

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

    /** The Cell Shader used to render Widgets' Cells. */
    CellShader m_cell_shader;

    /* OpenGL buffers *************************************************************************************************/

    GLuint m_fragment_buffer;

    GLuint m_vertex_array;

    GLuint m_vertex_buffer;

private: // static fields
    /** The current RenderContext. */
    static RenderContext* s_current_context;
};

} // namespace notf
