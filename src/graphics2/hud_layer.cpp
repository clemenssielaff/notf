#include "graphics2/hud_layer.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
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

void HUDLayer::begin_frame(const int width, const int height) // called from "beginFrame"
{
    m_calls.clear();
    m_paths.clear();

    m_viewport_size.width  = static_cast<float>(width);
    m_viewport_size.height = static_cast<float>(height);
}

void HUDLayer::abort_frame()
{
    m_calls.clear();
    m_paths.clear();
    //  gl->nverts = 0;
    //	gl->nuniforms = 0;
}

void end_frame()
{
}

void HUDLayer::add_fill_call(const Paint& paint, const Scissor& scissor, float fringe, const Aabr& bounds,
                             const std::vector<Path>& paths)
{
    m_calls.emplace_back();
    HUDCall& call   = m_calls.back();
    call.pathOffset = m_paths.size();
    call.pathCount  = paths.size();
    if (paths.size() == 1 && paths[0].is_convex) {
        call.type = HUDCall::Type::CONVEX_FILL;
    }
    else {
        call.type = HUDCall::Type::FILL;
    }

    //  this block is its own function in nanovg, called maxVertCount
    size_t new_vertex_count = 6; // + 6 for the quad that we construct further down
    for (const Path& path : paths) {
        new_vertex_count += path.fill.size();
        new_vertex_count += path.stroke.size();
    }
    //

    size_t offset = m_vertices.size();
    m_vertices.reserve(m_vertices.size() + new_vertex_count);

    m_paths.reserve(m_paths.size() + paths.size());
    for (const Path& path : paths) {
        HUDPath hud_path;
        if (!path.fill.empty()) {
            hud_path.fillOffset = offset;
            hud_path.fillCount  = path.fill.size();
            append(m_vertices, path.fill);
            offset += path.fill.size();
        }
        if (!path.stroke.empty()) {
            hud_path.strokeOffset = offset;
            hud_path.strokeCount  = path.stroke.size();
            append(m_vertices, path.stroke);
            offset += path.stroke.size();
        }
        m_paths.emplace_back(std::move(hud_path));
    }

    // create a quad around the bounds of the filled area
    assert(offset == m_vertices.size() - 6);
    call.triangleOffset = offset;
    call.triangleCount  = 6;
    m_vertices.emplace_back(bounds.left(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.top(), .5f, 1.f);

    m_vertices.emplace_back(bounds.left(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.top(), .5f, 1.f);
    m_vertices.emplace_back(bounds.left(), bounds.top(), .5f, 1.f);


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
