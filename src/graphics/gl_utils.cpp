#include "graphics/gl_utils.hpp"

#include "common/log.hpp"
#include "core/opengl.hpp"

namespace notf {

void gl_log_system_info()
{
    log_info << "OpenGL ES version:    " << glGetString(GL_VERSION);
    log_info << "OpenGL ES vendor:     " << glGetString(GL_VENDOR);
    log_info << "OpenGL ES renderer:   " << glGetString(GL_RENDERER);
    log_info << "OpenGL ES extensions: " << glGetString(GL_EXTENSIONS);
    log_info << "GLSL version:         " << glGetString(GL_SHADING_LANGUAGE_VERSION);
}

} // namespace notf
