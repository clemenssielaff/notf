#include "graphics2/hud_layer.hpp"

#include "common/log.hpp"
#include "core/glfw_wrapper.hpp"
#include "graphics2/backend.hpp"
#include "graphics2/hud_shader.hpp"

namespace notf {

HUDShader HUDLayer::s_shader = {}; // invalid by default

HUDLayer::HUDLayer(const RenderBackend& backend)
    : m_backend(backend)
    , m_viewport_size(Size2f(0, 0))
    , m_stencil_mask(0xffffffff)
    , m_stencil_func(StencilFunc::ALWAYS)
    , m_calls()
{
    if (!s_shader.is_valid()) {
        s_shader = HUDShader(backend);
        if (!s_shader.is_valid()) {
            log_critical << "Failed to construct the HUD shader!";
        }
    }
}

void HUDLayer::abort_frame()
{
    //    gl->nverts = 0;
    //	gl->npaths = 0;
    //	gl->ncalls = 0;
    //	gl->nuniforms = 0;
}

void end_frame()
{
}

void HUDLayer::set_stencil_mask(const GLuint mask)
{
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void HUDLayer::set_stencil_func(const StencilFunc func)
{
    if (func == m_stencil_func) {
        return;
    }
    switch (func) {
    case StencilFunc::ALWAYS:
        glStencilFunc(GL_ALWAYS, 0x00, 0xff);
        break;
    case StencilFunc::NEVER:
        glStencilFunc(GL_NEVER, 0x00, 0xff);
        break;
    case StencilFunc::LESS:
        glStencilFunc(GL_LESS, 0x00, 0xff);
        break;
    case StencilFunc::LEQUAL:
        glStencilFunc(GL_LEQUAL, 0x00, 0xff);
        break;
    case StencilFunc::GREATER:
        glStencilFunc(GL_GREATER, 0x00, 0xff);
        break;
    case StencilFunc::GEQUAL:
        glStencilFunc(GL_GEQUAL, 0x00, 0xff);
        break;
    case StencilFunc::EQUAL:
        glStencilFunc(GL_EQUAL, 0x00, 0xff);
        break;
    case StencilFunc::NOTEQUAL:
        glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
        break;
    }
}

} // namespace notf
