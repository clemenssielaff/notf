#pragma once

#include <memory>

#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "graphics/cell/painterpreter.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class FontManager;
struct Paint;
class Painterpreter;
class GraphicsContext;
struct Scissor;
class Shader;
class Texture2;

/**********************************************************************************************************************/

/** All values that determine the paint operations in the painted Cells.
 * We need options to stay the same during a frame, which is why they are collected from various sources at the
 * beginning and do not change until the next frame.
 */
struct CellCanvasOptions {

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float distance_tolerance;

    /** Tesselation density when creating rounded shapes. */
    float tesselation_tolerance;

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float fringe_width;

    /** See `GraphicsContextOptions` for details. */
    bool geometric_aa;

    /** See `GraphicsContextOptions` for details. */
    bool stencil_strokes;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f buffer_size;

    /** The mouse position relative to the Window's top-left corner. */
    Vector2f mouse_pos;

    /** Time at the beginning of the current frame. */
    Time time;
};

/**********************************************************************************************************************/

class CellCanvas {

    friend class Painterpreter;

private: // classes
    /******************************************************************************************************************/
    struct Path {
        GLint fill_offset    = 0;
        GLsizei fill_count   = 0;
        GLint stroke_offset  = 0;
        GLsizei stroke_count = 0;
    };

    /******************************************************************************************************************/

    struct Call {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
            TEXT,
        };
        Type type;
        size_t path_offset;
        size_t path_count;
        GLintptr uniform_offset;
        std::shared_ptr<Texture2> texture;
        GLint polygon_offset;
        GLsizei polygon_count;
    };

    /******************************************************************************************************************/

    struct ShaderVariables {
        enum class Type : GLint {
            GRADIENT = 0,
            IMAGE    = 1,
            STENCIL  = 2,
            TEXT     = 3,
        };

        float paint_2x2[4];
        float scissor_2x2[4];
        float paint_trans[2];
        float scissor_trans[2];
        float scissor_extent[2];
        float scissor_scale[2];
        Color inner_color;
        Color outer_color;
        float paint_extent[2];
        float radius;
        float feather;
        float stroke_factor;
        float stroke_threshold;
        Type type;
        float _padding;
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
        GLint image;

        /** Location of the `variables` uniform in the Shader. */
        GLuint variables;
    };

public: // methods
    /******************************************************************************************************************/
    /** Constructor. */
    CellCanvas(GraphicsContext& context);

    /** Destructor. */
    ~CellCanvas();

    /** The Font Manager. */
    const FontManager& get_font_manager() const;

    /** Returns the Cell options that are guaranteed to be consistent during the drawing of a frame. */
    const CellCanvasOptions& get_options() const { return m_options; }

    /** Begins a new frame. */
    void begin_frame(const Size2i& buffer_size, const Time time, const Vector2f mouse_pos);

    /** Paints a given Cell. */
    void paint(Cell& cell) { m_painterpreter->paint(cell); }
    void paint(Cell& cell, Xform2f base_xform, Scissor base_scissor)
    {
        m_painterpreter->paint(cell, std::move(base_xform), std::move(base_scissor));
    }

    /** Aborts the drawing of the current frame if something went wrong. */
    void reset();

    /** Performs all stored Calls. */
    void finish_frame();

private: // methods
    /** Fills a simple, convex shape. */
    void _perform_convex_fill(const Call& call);

    /** Fills multiple or complex shapes in one call. */
    void _perform_fill(const Call& call);

    /** Strokes a path. */
    void _perform_stroke(const Call& call);

    /** Renders a given text. */
    void _render_text(const Call& call);

    /** Writes the contents of a current frame to the log.
     * Is very long ... you should probably only do this once per run.
     */
    void _dump_debug_info() const;

private: // static methods
    /** Size (in bytes) of a ShaderVariables struct. */
    static constexpr GLintptr fragmentSize() { return sizeof(ShaderVariables); }

private: // fields
    /** Graphics Context used by the Cell Context. */
    GraphicsContext& m_graphics_context;

    /** The single Painterpreter used to paint in this Cell Context. */
    std::unique_ptr<Painterpreter> m_painterpreter;

    /** All values that determine the paint operations in the painted Cells. */
    CellCanvasOptions m_options;

    /** The Cell Shader used to render Widgets' Cells. */
    CellShader m_cell_shader;

    /** All Calls that were collected during during the frame. */
    std::vector<Call> m_calls;

    /** Indices of `m_vertices` of all Paths drawn during the frame. */
    std::vector<Path> m_paths;

    /** Vertices in screen space. */
    std::vector<Vertex> m_vertices;

    /** ShaderVariables for each Call. */
    std::vector<ShaderVariables> m_shader_variables;

    /** Buffer containing all fragment shader uniforms. */
    GLuint m_fragment_buffer;

    GLuint m_vertex_array;

    GLuint m_vertex_buffer;
};

} // namespace notf
