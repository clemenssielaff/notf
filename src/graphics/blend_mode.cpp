#include "graphics/blend_mode.hpp"

#include <utility>

#include "core/opengl.hpp"

namespace notf {

BlendMode::BlendMode()
    : rgb(SOURCE_OVER)
    , alpha(SOURCE_OVER)
{
}

BlendMode::BlendMode(const Mode mode)
    : rgb(mode)
    , alpha(std::move(mode))
{
}

BlendMode::BlendMode(const Mode color, const Mode alpha)
    : rgb(std::move(color))
    , alpha(std::move(alpha))
{
}

void BlendMode::apply() const
{
    // color
    GLenum rgb_sfactor;
    GLenum rgb_dfactor;
    switch (rgb) {
    case (SOURCE_OVER):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (SOURCE_IN):
        rgb_sfactor = GL_DST_ALPHA;
        rgb_dfactor = GL_ZERO;
        break;
    case (SOURCE_OUT):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ZERO;
        break;
    case (SOURCE_ATOP):
        rgb_sfactor = GL_DST_ALPHA;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (DESTINATION_OVER):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ONE;
        break;
    case (DESTINATION_IN):
        rgb_sfactor = GL_ZERO;
        rgb_dfactor = GL_SRC_ALPHA;
        break;
    case (DESTINATION_OUT):
        rgb_sfactor = GL_ZERO;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (DESTINATION_ATOP):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_SRC_ALPHA;
        break;
    case (LIGHTER):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ONE;
        break;
    case (COPY):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ZERO;
        break;
    case (XOR):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }

    // alpha
    GLenum alpha_sfactor;
    GLenum alpha_dfactor;
    switch (alpha) {
    case (SOURCE_OVER):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (SOURCE_IN):
        alpha_sfactor = GL_DST_ALPHA;
        alpha_dfactor = GL_ZERO;
        break;
    case (SOURCE_OUT):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ZERO;
        break;
    case (SOURCE_ATOP):
        alpha_sfactor = GL_DST_ALPHA;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (DESTINATION_OVER):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ONE;
        break;
    case (DESTINATION_IN):
        alpha_sfactor = GL_ZERO;
        alpha_dfactor = GL_SRC_ALPHA;
        break;
    case (DESTINATION_OUT):
        alpha_sfactor = GL_ZERO;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (DESTINATION_ATOP):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_SRC_ALPHA;
        break;
    case (LIGHTER):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ONE;
        break;
    case (COPY):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ZERO;
        break;
    case (XOR):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }

    glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor);
}

} // namespace notf
