#include "core/components/render_component.hpp"

#include <glad/glad.h>

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/shader.hpp"

namespace notf {

RenderComponent::RenderComponent(std::shared_ptr<Shader> shader)
    : Component()
    , m_shader(std::move(shader))
{

}

bool RenderComponent::is_valid()
{
    if (!m_shader || !glIsProgram(m_shader->get_id())) {
        log_critical << "Cannot create a RenderComponent with an invalid Shader";
        return false;
    }
    return true;
}

bool RenderComponent::assert_uniform(const char* name) const
{
    static const std::string gl_prefix = "gl_";
    if (starts_with(name, gl_prefix)) {
        log_critical << "Cannot use uniform variable " << name
                     << "' that starts with the reserved prefix: '" << gl_prefix
                     << "' in Shader: '" << m_shader->get_name() << "'";
        return false;
    }

    GLint location = glGetUniformLocation(m_shader->get_id(), name);
    if (check_gl_error()) {
        log_critical << "OpenGL error when checking Shader variable: '" << name << "'";
        return false;
    }
    if (location == -1) {
        log_critical << "Missing uniform '" << name << "' in Shader '" << m_shader->get_name() << "'";
        return false;
    }

    return true;
}

} // namespace notf
