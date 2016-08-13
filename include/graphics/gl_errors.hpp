#pragma once

#include "core/glfw_wrapper.hpp"

namespace signal {

/*!
 * @brief String representation of an OpenGl error code.
 *
 * @remark Is a constant expression so it can be used as optimizable input to log_* - functions.
 *
 * @param error_code    The OpenGl error code.
 *
 * @return The given error code as a string, empty if GL_NO_ERROR was passed.
 */
constexpr const char* gl_error_string(GLenum error_code)
{
    const char* invalid_enum = "GL_INVALID_ENUM";
    const char* invalid_value = "GL_INVALID_VALUE";
    const char* invalid_operation = "GL_INVALID_OPERATION";
    const char* invalid_framebuffer_operation = "GL_INVALID_FRAMEBUFFER_OPERATION";
    const char* out_of_memory = "GL_OUT_OF_MEMORY";

    switch(error_code){
    case GL_INVALID_ENUM:
        return invalid_enum;
    case GL_INVALID_VALUE:
        return invalid_value;
    case GL_INVALID_OPERATION:
        return invalid_operation;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return invalid_framebuffer_operation;
    case GL_OUT_OF_MEMORY:
        return out_of_memory;
    default:
        return "";
    }
}

} // namespace signal
