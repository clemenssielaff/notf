#include "graphics/hud_primitives.hpp"

#include "core/glfw_wrapper.hpp"

namespace notf {

BlendMode::Arguments BlendMode::get_arguments() const
{
    Arguments result;
    switch(rgb){
    case(SOURCE_OVER):
        result.rgb_sfactor = GL_ONE;
        result.rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(SOURCE_IN):
        result.rgb_sfactor = GL_DST_ALPHA;
        result.rgb_dfactor = GL_ZERO;
        break;
    case(SOURCE_OUT):
        result.rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.rgb_dfactor = GL_ZERO;
        break;
    case(SOURCE_ATOP):
        result.rgb_sfactor = GL_DST_ALPHA;
        result.rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(DESTINATION_OVER):
        result.rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.rgb_dfactor = GL_ONE;
        break;
    case(DESTINATION_IN):
        result.rgb_sfactor = GL_ZERO;
        result.rgb_dfactor = GL_SRC_ALPHA;
        break;
    case(DESTINATION_OUT):
        result.rgb_sfactor = GL_ZERO;
        result.rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(DESTINATION_ATOP):
        result.rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.rgb_dfactor = GL_SRC_ALPHA;
        break;
    case(LIGHTER):
        result.rgb_sfactor = GL_ONE;
        result.rgb_dfactor = GL_ONE;
        break;
    case(COPY):
        result.rgb_sfactor = GL_ONE;
        result.rgb_dfactor = GL_ZERO;
        break;
    case(XOR):
        result.rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }
    switch(alpha){
    case(SOURCE_OVER):
        result.alpha_sfactor = GL_ONE;
        result.alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(SOURCE_IN):
        result.alpha_sfactor = GL_DST_ALPHA;
        result.alpha_dfactor = GL_ZERO;
        break;
    case(SOURCE_OUT):
        result.alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.alpha_dfactor = GL_ZERO;
        break;
    case(SOURCE_ATOP):
        result.alpha_sfactor = GL_DST_ALPHA;
        result.alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(DESTINATION_OVER):
        result.alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.alpha_dfactor = GL_ONE;
        break;
    case(DESTINATION_IN):
        result.alpha_sfactor = GL_ZERO;
        result.alpha_dfactor = GL_SRC_ALPHA;
        break;
    case(DESTINATION_OUT):
        result.alpha_sfactor = GL_ZERO;
        result.alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case(DESTINATION_ATOP):
        result.alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.alpha_dfactor = GL_SRC_ALPHA;
        break;
    case(LIGHTER):
        result.alpha_sfactor = GL_ONE;
        result.alpha_dfactor = GL_ONE;
        break;
    case(COPY):
        result.alpha_sfactor = GL_ONE;
        result.alpha_dfactor = GL_ZERO;
        break;
    case(XOR):
        result.alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        result.alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    }
    return result;
}


} // namespace notf
