#pragma once

#include <vector>

#include "common/size2f.hpp"
#include "graphics/gl_forwards.hpp"

namespace notf {

class HUDShader;
class RenderBackend;

class HUDCall {
    enum class Type : unsigned char {
        FILL,
        CONVEX_FILL,
        STROKE,
        TRIANGLES,
    };

    Type type;
    int image;
    int pathOffset;
    int pathCount;
    int triangleOffset;
    int triangleCount;
    int uniformOffset;
};

/**********************************************************************************************************************/

/** The HUDLayer is a RenderLayer specialized in rendering dynamic, 2D Widgets.
 * At the moment, the HUDLayer is the only reder layer in NoTF, however, I can easily imagine a 3D Layer, for example.
 * In that case, add an abstract parent class and implement the relevant virtual functions.
 */
class HUDLayer {

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

    void begin_frame(const int width, const int height) // called from "beginFrame"
    {
        m_viewport_size.width  = static_cast<float>(width);
        m_viewport_size.height = static_cast<float>(height);
    }

    void abort_frame();

    void end_frame();

private: // methods for HUDPainter
    void add_fill_call()

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

private: // static fields
    static HUDShader s_shader;
};

} // namespace notf
