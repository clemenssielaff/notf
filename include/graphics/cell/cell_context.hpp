#pragma once

#include <memory>

#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "graphics/cell/painterpreter.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/vertex.hpp"

namespace notf {

struct Paint;
class Painterpreter;
class GraphicsContext;
struct Scissor;
class Shader;
class Texture2;

class CellContext {

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
        };
        Type type;
        size_t path_offset;
        size_t path_count;
        GLintptr uniform_offset;
        std::shared_ptr<Texture2> texture;
        GLint polygon_offset;
    };

    /******************************************************************************************************************/

    struct alignas(8) ShaderVariables {
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
    CellContext(GraphicsContext& context);

    /** Destructor. */
    ~CellContext();

    /** The Painterpreter painting into the Cell Context. */
    Painterpreter& get_painterpreter() const { return *m_painterpreter.get(); }

    /** Time at the beginning of the current frame. */
    Time get_time() const { return m_time; }

    /** The mouse position relative to the Window's top-left corner. */
    const Vector2f& get_mouse_pos() const { return m_mouse_pos; }

    /** The pixel ratio of the GraphicsContext. */
    float get_pixel_ratio() const { return m_context.get_args().pixel_ratio; }

    /** Whether Cells should provide their own geometric antialiasing or not. */
    bool provides_geometric_aa() const { return m_context.get_args().geometric_aa; }

    /** Whether the fragment shader should discard antialiased stroke fragments below a certain threshold
     * See GraphicsContextArguments for details.
     */
    bool has_save_alpha_strokes() const { return m_context.get_args().save_alpha_stroke; }

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float get_distance_tolerance() const { return m_distance_tolerance; }

    /** Tesselation density when creating rounded shapes. */
    float get_tesselation_tolerance() const { return m_tesselation_tolerance; }

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float get_fringe_width() const { return m_fringe_width; }

    /** Begins a new frame. */
    void begin_frame(const Size2i& buffer_size, const Time time, const Vector2f mouse_pos);

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

    /** Writes the contents of a current frame to the log.
     * Is very long ... you should probably only do this once per run.
     */
    void _dump_debug_info() const;

private: // static methods
    /** Size (in bytes) of a ShaderVariables struct. */
    static constexpr GLintptr fragmentSize() { return sizeof(ShaderVariables); }

private: // fields
    /** Context used by the Cell Context. */
    GraphicsContext& m_context;

    /** The single Painterpreter used to paint in this Cell Context. */
    std::unique_ptr<Painterpreter> m_painterpreter;

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

    /* Paint parameters ***********************************************************************************************/

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float m_distance_tolerance;

    /** Tesselation density when creating rounded shapes. */
    float m_tesselation_tolerance;

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float m_fringe_width;

    /* Per-frame infos ************************************************************************************************/

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2f m_buffer_size;

    /** Time at the beginning of the current frame. */
    Time m_time;

    /** The mouse position relative to the Window's top-left corner. */
    Vector2f m_mouse_pos;

    /* OpenGL buffers *************************************************************************************************/

    /** Buffer containing all fragment shader uniforms. */
    GLuint m_fragment_buffer;

    GLuint m_vertex_array;

    GLuint m_vertex_buffer;
};

} // namespace notf
