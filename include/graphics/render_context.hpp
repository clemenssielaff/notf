#pragma once

#include <vector>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/size2i.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/canvas_cell.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/shader.hpp"
#include "graphics/vertex.hpp"

namespace notf {

struct RenderContextArguments;

/**********************************************************************************************************************/

struct RenderContextArguments {

    /** Flag indicating whether the RenderContext will provide geometric antialiasing for its 2D shapes or not.
     * In a purely 2D application, this flag should be set to `true` since geometric antialiasing is cheaper than
     * full blown multisampling and looks just as good.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we don't
     * need the redundant geometrical antialiasing on top.
     */
    bool enable_geometric_aa = true;

    /** Default to GLES_3, because it's the fastest, newest version that works on my machine.
     * (obviously, that's not at very good reason, but we'll see what happens).
     */
    OpenGLVersion version = OpenGLVersion::GLES_3;

    /** Pixel ratio of the RenderContext.
     * 1.0 means square pixels.
     */
    float pixel_ratio = 1.f; // TODO: I actually don't know if pixel_ratio is width/height or the other way around...
};

/**********************************************************************************************************************/

/** The RenderContext. */
class RenderContext {

    friend class FrameGuard;
    friend class Cell;

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
        GLsizei triangleCount;
        GLintptr uniformOffset;
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
            SIMPLE,
        };

        FragmentUniforms() // we'll see if we need this initialization to zero at all
            : scissorMat{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
              paintMat{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
              innerCol{0},
              outerCol{0},
              scissorExt{0, 0},
              scissorScale{0, 0},
              extent{0, 0},
              radius{0},
              feather{0},
              strokeMult{0},
              strokeThr{0},
              texType{0},
              type{Type::SIMPLE}
        {
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
        FrameGuard(RenderContext* context)
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
        RenderContext* m_canvas;
    };

public: // enum
    enum class StencilFunc : unsigned char {
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
    RenderContext(const RenderContextArguments args);

    ~RenderContext();

    FrameGuard begin_frame(const Size2i buffer_size);

    float get_pixel_ratio() const { return m_args.pixel_ratio; }

    bool provides_geometric_aa() const { return m_args.enable_geometric_aa; }

private: // methods for Cell
    void add_fill_call(const Paint& paint, const Cell& cell);

    void add_stroke_call(const Paint& paint, const float stroke_width, const Cell& cell);

    void set_stencil_mask(const GLuint mask);

    void set_stencil_func(const StencilFunc func);

private: // methods for FrameGuard
    void _abort_frame();

    void _end_frame();

private: // methods
    void _paint_to_frag(FragmentUniforms& frag, const Paint& paint, const Scissor& scissor,
                        const float stroke_width, const float fringe, const float stroke_threshold);

    void _render_flush(BlendMode blend_mode);

    void _fill(const CanvasCall& call);

    void _convex_fill(const CanvasCall& call);

    void _stroke(const CanvasCall& call);

private: // static methods
    static Sources _create_shader_sources(const RenderContext &context);

private: // fields
    /** Argument struct to initialize the RenderContext. */
    const RenderContextArguments m_args;

    /** Size of the Window in screen coordinates (not pixels). */
    Size2i m_window_size;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f m_buffer_size;

    /** Time at the beginning of the current frame. */
    Time m_time;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /** Cached stencil func to avoid unnecessary rebindings. */
    StencilFunc m_stencil_func;

    /** All Calls that were collected during during the frame. */
    std::vector<CanvasCall> m_calls;

    /** Indices of `m_vertices` of all Paths drawn during the frame. */
    std::vector<PathIndex> m_paths;

    /** Vertices (global, not path specific?) */
    std::vector<Vertex> m_vertices;

    /** Fragment uniform buffers. */
    std::vector<FragmentUniforms> m_frag_uniforms;

    // Shader variables

    Sources m_sources;

    Shader m_shader;

    GLint m_loc_viewsize;

    GLint m_loc_texture;

    GLuint m_loc_buffer;

    GLuint m_fragment_buffer;

    GLuint m_vertex_array;

    GLuint m_vertex_buffer;
};

} // namespace notf
