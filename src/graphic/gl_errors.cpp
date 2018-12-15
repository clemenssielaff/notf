#include "notf/graphic/gl_errors.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/glfw.hpp"

namespace { // anonymous

/// String representation of an OpenGl error code.
/// Is a constant expression so it can be used as optimizable input to log_* - functions.
/// @param error_code    The OpenGl error code.
/// @return              The given error code as a string, empty if GL_NO_ERROR was passed.
constexpr const char* gl_error_string(GLenum error_code) {
    const char* no_error = "GL_NO_ERROR";
    const char* invalid_enum = "GL_INVALID_ENUM";
    const char* invalid_value = "GL_INVALID_VALUE";
    const char* invalid_operation = "GL_INVALID_OPERATION";
    const char* out_of_memory = "GL_OUT_OF_MEMORY";
    const char* invalid_framebuffer_operation = "GL_INVALID_FRAMEBUFFER_OPERATION";
#ifdef GL_STACK_OVERFLOW
    const char* stack_overflow = "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
    const char* stack_underflow = "GL_STACK_UNDERFLOW";
#endif

    switch (error_code) {
    case GL_NO_ERROR: // No user error reported since last call to glGetError.
        return no_error;
    case GL_INVALID_ENUM: // Set when an enumeration parameter is not legal.
        return invalid_enum;
    case GL_INVALID_VALUE: // Set when a value parameter is not legal.
        return invalid_value;
    case GL_INVALID_OPERATION: // Set when the state for a command is not legal for its given parameters.
        return invalid_operation;
    case GL_OUT_OF_MEMORY: // Set when a memory allocation operation cannot allocate (enough) memory.
        return out_of_memory;
    case GL_INVALID_FRAMEBUFFER_OPERATION: // Set when reading or writing to a framebuffer that is not complete.
        return invalid_framebuffer_operation;
#ifdef GL_STACK_OVERFLOW
    case GL_STACK_OVERFLOW: // Set when a stack pushing operation causes a stack overflow.
        return stack_overflow;
#endif
#ifdef GL_STACK_UNDERFLOW
    case GL_STACK_UNDERFLOW: // Set when a stack popping operation occurs while the stack is at its lowest point.
        return stack_underflow;
#endif
    default: return "";
    }
}

} // namespace

NOTF_OPEN_NAMESPACE

// opengl error handling ============================================================================================ //

namespace detail {

void check_gl_error(uint line, const char* file) {
    if (glfwGetCurrentContext() == nullptr) {
        NOTF_THROW(OpenGLError, "No OpenGL context current on this thread");
    }

    int error_count = 0;
    GLenum error_code;
    while ((error_code = glGetError()) != GL_NO_ERROR) {
        ++error_count;
        ::notf::TheLogger::get()->warn("OpenGL error: {} ({}:{})", gl_error_string(error_code),
                                       ::notf::filename_from_path(file), line);
    }
}

} // namespace detail

void clear_gl_errors() {
    if constexpr (config::is_debug_build()) {
        while (glGetError() != GL_NO_ERROR) {};
    }
}

NOTF_CLOSE_NAMESPACE