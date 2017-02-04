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

class HUDShader;
class RenderBackend;

/**********************************************************************************************************************/

/** The CanvasLayer is a RenderLayer specialized in rendering dynamic, 2D Widgets.
 * At the moment, the CanvasLayer is the only reder layer in NoTF, however, I can easily imagine a 3D Layer, for example.
 * In that case, add an abstract parent class and implement the relevant virtual functions.
 */
class CanvasLayer {

    friend class FrameGuard;
    friend class Cell;

    struct HUDCall {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
        };

        HUDCall() // we'll see if we need this initialization to zero at all
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

    constexpr GLintptr fragSize()
    {
        constexpr GLintptr align = sizeof(float);
        return sizeof(FragmentUniforms) + align - sizeof(FragmentUniforms) % align;
    }

    struct Sources {
        std::string vertex;
        std::string fragment;
    };

    /******************************************************************************************************************/
    /** The FrameGuard makes sure that for each call to `CanvasLayer::begin_frame` there is a corresponding call to
     * either `CanvasLayer::end_frame` on success or `CanvasLayer::abort_frame` in case of an error.
     *
     * It is returned by `CanvasLayer::begin_frame` and must remain on the stack until the rendering has finished.
     * Then, you need to call `FrameGuard::end()` to cleanly end the frame.
     * If the FrameGuard is destroyed before `FrameGuard::end()` is called, the CanvasLayer is instructed to abort the
     * currently drawn frame.
     */
    class FrameGuard {

    public: // methods
        /** Constructor. */
        FrameGuard(CanvasLayer* context)
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
         * If the object is destroyed before FrameGuard::end() is called, the CanvasLayer's frame is cancelled.
         */
        ~FrameGuard()
        {
            if (m_canvas) {
                m_canvas->_abort_frame();
            }
        }

        /** Cleanly ends the HUDCanvas's current frame. */
        void end()
        {
            if (m_canvas) {
                m_canvas->_end_frame();
                m_canvas = nullptr;
            }
        }

    private: // fields
        /** CanvasLayer currently drawing a frame.*/
        CanvasLayer* m_canvas;
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
    CanvasLayer(const RenderBackend& backend, const float pixel_ratio = 1);

    ~CanvasLayer();

    FrameGuard begin_frame(const int width, const int height); // called from "beginFrame"

    float get_pixel_ratio() const { return m_pixel_ratio; }

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

    void _fill(const HUDCall& call);

    void _convex_fill(const HUDCall& call);

    void _stroke(const HUDCall& call);

private: // methods
    static Sources _create_shader_sources(const RenderBackend& render_backend);

public: // fields
    const RenderBackend& backend;

private: // fields
    /** Size of the Window in screen coordinates (not pixels). */
    Size2i window_size;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f m_buffer_size;

    float m_pixel_ratio;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /** Cached stencil func to avoid unnecessary rebindings. */
    StencilFunc m_stencil_func;

    /** All Calls that were collected during during the frame. */
    std::vector<HUDCall> m_calls;

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
