#include "graphics/graphics_context.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/glfw.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/texture2.hpp"

namespace notf {

GraphicsContext* GraphicsContext::s_current_context = nullptr;

GraphicsContext::GraphicsContext(GLFWwindow* window)
    : m_window(window)
    , m_current_thread(0)
    , m_has_vsync(true)
    , m_stencil_func(StencilFunc::INVALID)
    , m_stencil_mask(0)
    , m_blend_mode(BlendMode::INVALID)
    , m_textures()
    , m_shader_stack()
    , m_shaders()
    , m_font_manager()
{
    if (!window) {
        throw_runtime_error(
            "Failed to create a new GraphicsContext without a window (given pointer is null).");
    }

    GLFWwindow* current_context = glfwGetCurrentContext();
    if (current_context) {
        assert(s_current_context->m_window == current_context);
        throw_runtime_error(
            "Failed to create a new GraphicsContext instance with another one being current.");
    }

    make_current();
    glfwSwapInterval(m_has_vsync ? 1 : 0);

    m_font_manager = std::make_unique<FontManager>(*this);
}

GraphicsContext::~GraphicsContext()
{
    m_font_manager.reset();

    // deallocate and invalidate all remaining Textures
    for (std::weak_ptr<Texture2> texture_weakptr : m_textures) {
        std::shared_ptr<Texture2> texture = texture_weakptr.lock();
        if (texture) {
            log_warning << "Deallocating live Texture: \"" << texture->name() << "\"";
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (std::weak_ptr<Shader> shader_weakptr : m_shaders) {
        std::shared_ptr<Shader> shader = shader_weakptr.lock();
        if (shader) {
            log_warning << "Deallocating live Shader: \"" << shader->name() << "\"";
            shader->_deallocate();
        }
    }
    m_shaders.clear();
}

void GraphicsContext::make_current()
{
    if (s_current_context != this) {
        glfwMakeContextCurrent(m_window);
        s_current_context = this;
        m_current_thread  = std::this_thread::get_id();
    }
}

bool GraphicsContext::is_current() const
{
    return this == s_current_context && m_current_thread == std::this_thread::get_id();
}

void GraphicsContext::set_vsync(const bool enabled)
{
    if (enabled == m_has_vsync) {
        return;
    }
    if (!is_current()) {
        throw_runtime_error("Cannot change vsyn of a graphics context that is not current");
    }
    m_has_vsync = enabled;
    glfwSwapInterval(m_has_vsync ? 1 : 0);
}

void GraphicsContext::set_stencil_func(const StencilFunc func)
{
    if (!is_current()) {
        throw_runtime_error("Cannot change the stencil func of a graphics context that is not current");
    }
    if (func != m_stencil_func) {
        m_stencil_func = func;
        m_stencil_func.apply();
    }
}

void GraphicsContext::set_stencil_mask(const GLuint mask)
{
    if (!is_current()) {
        throw_runtime_error("Cannot change the stencil mask of a graphics context that is not current");
    }
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void GraphicsContext::set_blend_mode(const BlendMode mode)
{
    if (!is_current()) {
        throw_runtime_error("Cannot change the blend mode of a graphics context that is not current");
    }
    if (mode != m_blend_mode) {
        m_blend_mode = mode;
        m_blend_mode.apply();
    }
}

void GraphicsContext::push_texture(Texture2Ptr texture)
{
    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind texture \"%s\" with a graphics context that is not current",
            texture->name().c_str()));
    }
    if (!texture->is_valid()) {
        throw_runtime_error(string_format(
            "Cannot bind invalid texture \"%s\"",
            texture->name().c_str()));
    }

    if (m_texture_stack.empty() || texture != m_texture_stack.back()) {
        glBindTexture(GL_TEXTURE_2D, texture->id());
    }
    m_texture_stack.emplace_back(std::move(texture));
}

void GraphicsContext::pop_texture()
{
    if (m_texture_stack.empty()) {
        return; // ignore calls on an empty stack
    }

    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind texture  \"%s\" with a graphics context that is not current",
            m_texture_stack.back()->name().c_str()));
    }

    const Texture2Ptr last_texture = take_back(m_texture_stack);
    if (m_texture_stack.empty()) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else if (last_texture != m_texture_stack.back()) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(m_texture_stack.back()->id());
    }
}

void GraphicsContext::clear_texture()
{
    if (m_texture_stack.empty()) {
        return; // ignore calls on an empty stack
    }

    if (!is_current()) {
        throw_runtime_error("Cannot unbind textures from a graphics context that is not current");
    }

    m_texture_stack.clear();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GraphicsContext::push_shader(ShaderPtr shader)
{
    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind shader  \"%s\" with a graphics context that is not current",
            shader->name().c_str()));
    }
    if (!shader->is_valid()) {
        throw_runtime_error(string_format(
            "Cannot bind invalid shader \"%s\"",
            shader->name().c_str()));
    }

    if (m_shader_stack.empty() || shader != m_shader_stack.back()) {
        glUseProgram(shader->id());
    }
    m_shader_stack.emplace_back(std::move(shader));
}

void GraphicsContext::pop_shader()
{
    if (m_shader_stack.empty()) {
        return; // ignore calls on an empty stack
    }

    if (!is_current()) {
        throw_runtime_error(string_format(
            "Cannot bind shader  \"%s\" with a graphics context that is not current",
            m_shader_stack.back()->name().c_str()));
    }

    const ShaderPtr last_shader = take_back(m_shader_stack);
    if (m_shader_stack.empty()) {
        glUseProgram(0);
    }
    else if (last_shader != m_shader_stack.back()) {
        glUseProgram(m_shader_stack.back()->id());
    }
}

void GraphicsContext::clear_shader()
{
    if (m_shader_stack.empty()) {
        return; // ignore calls on an empty stack
    }

    if (!is_current()) {
        throw_runtime_error("Cannot unbind shaders from a graphics context that is not current");
    }

    m_shader_stack.clear();
    glUseProgram(0);
}

} // namespace notf
