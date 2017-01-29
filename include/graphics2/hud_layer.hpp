#pragma once

#include <vector>

#include "common/aabr.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics2/hud_primitives.hpp"

namespace notf {

class HUDShader;
class RenderBackend;

/**********************************************************************************************************************/

/** The HUDLayer is a RenderLayer specialized in rendering dynamic, 2D Widgets.
 * At the moment, the HUDLayer is the only reder layer in NoTF, however, I can easily imagine a 3D Layer, for example.
 * In that case, add an abstract parent class and implement the relevant virtual functions.
 */
class HUDLayer {

    struct HUDCall {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
            TRIANGLES,
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
        size_t triangleOffset;
        size_t triangleCount;
        size_t uniformOffset;
    };

    struct HUDPath {

        HUDPath() // we'll see if we need this initialization to zero at all
            : fillOffset(0),
              fillCount(0),
              strokeOffset(0),
              strokeCount(0)
        {
        }

        size_t fillOffset;
        size_t fillCount;
        size_t strokeOffset;
        size_t strokeCount;
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
    HUDLayer(const RenderBackend& backend);

    void begin_frame(const int width, const int height); // called from "beginFrame"

    void abort_frame();

    void end_frame();

private: // methods for HUDPainter
    void add_fill_call(const Paint& paint, const Scissor& scissor, float fringe, const Aabr& bounds,
                       const std::vector<Path>& paths);

    void set_stencil_mask(const GLuint mask);

    void set_stencil_func(const StencilFunc func);

private: // fields
    const RenderBackend& m_backend;

    Size2f m_viewport_size;

    /* Cached stencil mask to avoid unnecessary rebindings. */
    GLuint m_stencil_mask;

    /** Cached stencil func to avoid unnecessary rebindings. */
    StencilFunc m_stencil_func;

    /** All Calls that were collected during during the frame. */
    std::vector<HUDCall> m_calls;

    /** All Paths drawn during the frame. */
    std::vector<HUDPath> m_paths;

    /** Vertices (global, not path specific?) */
    std::vector<Vertex> m_vertices;

private: // static fields
    static HUDShader s_shader;
};

} // namespace notf
