#pragma once

#include <memory>
#include <vector>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/cell_old.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class Texture2;
class Window;

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
class RenderContext_Old {

    friend class Cell_Old;
    friend class FrameGuard;
    friend class RenderManager;
    friend class Shader;
    friend class Texture2;

public:
    struct CanvasCall {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
        };

        CanvasCall() // we'll see if we need this initialization to zero at all
            : type(Type::FILL),
              pathOffset(0),
              pathCount(0),
              triangleOffset(0),
              triangleCount(0),
              uniformOffset(0)
        {
        }

        Type type;
        size_t pathOffset;
        size_t pathCount;
        GLint triangleOffset;
        GLsizei triangleCount; // is always 6
        GLintptr uniformOffset;
        std::shared_ptr<Texture2> texture;
    };

    struct PathIndex {

        PathIndex() // we'll see if we need this initialization to zero at all
            : fillOffset(0),
              fillCount(0),
              strokeOffset(0),
              strokeCount(0)
        {
        }

        GLint fillOffset;
        GLsizei fillCount;
        GLint strokeOffset;
        GLsizei strokeCount;
    };

    struct FragmentUniforms { //  TODO: replace more of the float[]s with explicit types
        enum class Type : GLint {
            GRADIENT,
            IMAGE,
            SIMPLE,
        };

        FragmentUniforms() // we'll see if we need this initialization to zero at all
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

    struct Sources {
        std::string vertex;
        std::string fragment;
    };

public: // classes
    /******************************************************************************************************************/
    /** The FrameGuard makes sure that for each call to `CanvasContext::begin_frame` there is a corresponding call to
     * either `CanvasContext::end_frame` on success or `CanvasContext::abort_frame` in case of an error.
     *
     * It is returned by `CanvasContext::begin_frame` and must remain on the stack until the rendering has finished.
     * Then, you need to call `FrameGuard::end()` to cleanly end the frame.
     * If the FrameGuard is destroyed before `FrameGuard::end()` is called, the CanvasContext is instructed to abort the
     * currently drawn frame.
     */
    class FrameGuard {

    public: // methods
        /** Constructor. */
        FrameGuard(RenderContext_Old* context)
            : m_canvas(context) {}

        // no copy/assignment
        FrameGuard(const FrameGuard&) = delete;
        FrameGuard& operator=(const FrameGuard&) = default;

        /** Move Constructor. */
        FrameGuard(FrameGuard&& other)
            : m_canvas(other.m_canvas)
        {
            other.m_canvas = nullptr;
        }

        /** Destructor.
         * If the object is destroyed before FrameGuard::end() is called, the CanvasContext's frame is cancelled.
         */
        ~FrameGuard()
        {
            if (m_canvas) {
                m_canvas->_abort_frame();
            }
        }

        /** Cleanly ends the current frame. */
        void end()
        {
            if (m_canvas) {
                m_canvas->_end_frame();
                m_canvas = nullptr;
            }
        }

    private: // fields
        /** CanvasContext currently drawing a frame.*/
        RenderContext_Old* m_canvas;
    };

public: // enum
    enum class StencilFunc_Old : unsigned char {
        ALWAYS,
        NEVER,
        LESS,
        LEQUAL,
        GREATER,
        GEQUAL,
        EQUAL,
        NOTEQUAL,
    };

public:
    /** Constructor. */
    RenderContext_Old(const Window* window, const RenderContextArguments args);

    /** Destructor. */
    ~RenderContext_Old();

    /** Makes the OpenGL context of this RenderContext current. */
    void make_current();

    FrameGuard begin_frame(const Size2i buffer_size);

    float get_pixel_ratio() const { return m_args.pixel_ratio; }

    float get_distance_tolerance() const { return 0.01f / m_args.pixel_ratio; }

    float get_tesselation_tolerance() const { return 0.25f / m_args.pixel_ratio; }

    float get_fringe_width() const { return 1.f / m_args.pixel_ratio; }

    bool provides_geometric_aa() const { return m_args.enable_geometric_aa; }

    const Vector2f& get_mouse_pos() const { return m_mouse_pos; }

    Time get_time() const { return m_time; }

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

private: // methods for RenderManager
    void set_mouse_pos(Vector2f pos) { m_mouse_pos = std::move(pos); }

    void set_buffer_size(Size2f buffer) { m_buffer_size = std::move(buffer); }

private: // methods for Cell
    void add_fill_call(const Paint& paint, const Cell_Old& cell);

    void add_stroke_call(const Paint& paint, const float stroke_width, const Cell_Old& cell);

    void set_stencil_mask(const GLuint mask);

    void set_stencil_func(const StencilFunc_Old func);

private: // methods for FrameGuard
    void _abort_frame();

    void _end_frame();

private: // methods
    void _paint_to_frag(FragmentUniforms& frag, const Paint& paint, const Scissor_Old& scissor,
                        const float stroke_width, const float fringe, const float stroke_threshold);

    void _render_flush(BlendMode blend_mode);

    void _fill(const CanvasCall& call);

    void _convex_fill(const CanvasCall& call);

    void _stroke(const CanvasCall& call);

private: // methods for friends
    /** Binds the Texture with the given ID, but only if it is not the currently bound one. */
    void _bind_texture(const GLuint texture_id);

    /** Binds the Shader with the given ID, but only if it is not the currently bound one. */
    void _bind_shader(const GLuint shader_id);

private: // static methods
    static Sources _create_shader_sources(const RenderContext_Old& context);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** Argument struct to initialize the RenderContext. */
    RenderContextArguments m_args;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f m_buffer_size;

    /** Time at the beginning of the current frame. */
    Time m_time;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /** Cached stencil func to avoid unnecessary rebindings. */
    StencilFunc_Old m_stencil_func;

    /** All Calls that were collected during during the frame. */
    std::vector<CanvasCall> m_calls;

    /** Indices of `m_vertices` of all Paths drawn during the frame. */
    std::vector<PathIndex> m_paths;

    /** Vertices (global, not path specific?) */
    std::vector<Vertex> m_vertices;

    /** Fragment uniform buffers. */
    std::vector<FragmentUniforms> m_frag_uniforms;

    /** Position of the mouse relative to the Window. */
    Vector2f m_mouse_pos;

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

    /* Cell Shader ****************************************************************************************************/

    Sources m_sources;

    std::shared_ptr<Shader> m_cell_shader;

    GLint m_loc_viewsize;

    GLint m_loc_texture;

    GLuint m_loc_buffer;

    GLuint m_fragment_buffer;

    GLuint m_vertex_array;

    GLuint m_vertex_buffer;

private: // static fields
    /** The current RenderContext. */
    static RenderContext_Old* s_current_context;
};

} // namespace notf
