#include "graphics/graphics_context.hpp"

#include "common/log.hpp"
#include "core/glfw.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/texture2.hpp"

namespace notf {

GraphicsContext* GraphicsContext::s_current_context = nullptr;

GraphicsContext::GraphicsContext(const Window* window)
    : m_window(window)
    , m_current_thread(0)
    , m_stencil_func(StencilFunc::INVALID)
    , m_stencil_mask(0)
    , m_blend_mode(BlendMode::INVALID)
    , m_bound_texture(0)
    , m_textures()
    , m_bound_shader(0)
    , m_shaders()
    , m_font_manager()
{
    if (!window) {
        throw_runtime_error(
            "Failed to create a new GraphicsContext without a window (given pointer is null).");
    }
    if (s_current_context) {
        throw_runtime_error(
            "Failed to create a new GraphicsContext instance with another one being current.");
    }
    if (glfwGetCurrentContext() != m_window->_get_glfw_window()) {
        throw_runtime_error(
            "Failed to create a new GraphicsContext with a window that does not have the current OpenGL context.");
    }

    make_current();
    m_font_manager = std::make_unique<FontManager>(*this);
}

GraphicsContext::~GraphicsContext()
{
    // deallocate and invalidate all remaining Textures
    for (std::weak_ptr<Texture2> texture_weakptr : m_textures) {
        std::shared_ptr<Texture2> texture = texture_weakptr.lock();
        if (texture) {
            log_warning << "Deallocating live Texture: " << texture->name();
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (std::weak_ptr<Shader> shader_weakptr : m_shaders) {
        std::shared_ptr<Shader> shader = shader_weakptr.lock();
        if (shader) {
            log_warning << "Deallocating live Shader: " << shader->name();
            shader->_deallocate();
        }
    }
    m_shaders.clear();
}

void GraphicsContext::make_current()
{
    if (s_current_context != this) {
        glfwMakeContextCurrent(m_window->_get_glfw_window());
        s_current_context = this;
        m_current_thread  = std::this_thread::get_id();
    }
}

bool GraphicsContext::is_current() const
{
    return this == s_current_context && m_current_thread == std::this_thread::get_id();
}

void GraphicsContext::set_stencil_func(const StencilFunc func)
{
    if (func != m_stencil_func) {
        m_stencil_func = func;
        m_stencil_func.apply();
    }
}

void GraphicsContext::set_stencil_mask(const GLuint mask)
{
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void GraphicsContext::set_blend_mode(const BlendMode mode)
{
    if (mode != m_blend_mode) {
        m_blend_mode = mode;
        m_blend_mode.apply();
    }
}

void GraphicsContext::bind_texture(Texture2* texture)
{
    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind texture \"%s\" with a graphics context that is not current",
            texture->name().c_str()));
    }
    const GLuint texture_id = texture->id();
    if (texture_id != m_bound_texture) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        m_bound_texture = texture_id;
    }
}

void GraphicsContext::bind_shader(Shader* shader)
{
    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind shader  \"%s\" with a graphics context that is not current",
            shader->name().c_str()));
    }
    const GLuint shader_id = shader->id();
    if (shader_id != m_bound_shader) {
        glUseProgram(shader_id);
        m_bound_shader = shader_id;
    }
}

void GraphicsContext::force_reloads()
{
    m_stencil_func  = StencilFunc::INVALID;
    m_stencil_mask  = 0;
    m_blend_mode    = BlendMode::INVALID;
    m_bound_texture = 0;
    m_bound_shader  = 0;
}

} // namespace notf
