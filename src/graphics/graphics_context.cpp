#include "graphics/graphics_context.hpp"

#include "common/log.hpp"
#include "core/glfw.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/texture2.hpp"

namespace notf {

GraphicsContext* GraphicsContext::s_current_context = nullptr;

GraphicsContext::GraphicsContext(const Window* window, const GraphicsContextOptions args)
    : m_window(window)
    , m_options(std::move(args))
    , m_font_manager(std::make_unique<FontManager>(this))
    , m_stencil_func(StencilFunc::INVALID)
    , m_stencil_mask(0)
    , m_blend_mode(BlendMode::INVALID)
    , m_bound_texture(0)
    , m_textures()
    , m_bound_shader(0)
    , m_shaders()
{
    // make sure the pixel ratio is real and never zero
    if (!is_real(m_options.pixel_ratio)) {
        log_warning << "Pixel ratio cannot be " << m_options.pixel_ratio << ", defaulting to 1";
        m_options.pixel_ratio = 1;
    }
    else if (std::abs(m_options.pixel_ratio) < precision_high<float>()) {
        log_warning << "Pixel ratio cannot be zero, defaulting to 1";
        m_options.pixel_ratio = 1;
    }
}

GraphicsContext::~GraphicsContext()
{
    // deallocate and invalidate all remaining Textures
    for (std::weak_ptr<Texture2> texture_weakptr : m_textures) {
        std::shared_ptr<Texture2> texture = texture_weakptr.lock();
        if (texture) {
            log_warning << "Deallocating live Texture: " << texture->get_name();
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (std::weak_ptr<Shader> shader_weakptr : m_shaders) {
        std::shared_ptr<Shader> shader = shader_weakptr.lock();
        if (shader) {
            log_warning << "Deallocating live Shader: " << shader->get_name();
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
    }
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

std::shared_ptr<Texture2> GraphicsContext::load_texture(const std::string& file_path)
{
    std::shared_ptr<Texture2> texture = Texture2::load(this, file_path);
    if (texture) {
        m_textures.emplace_back(texture);
    }
    return texture;
}

std::shared_ptr<Shader> GraphicsContext::build_shader(const std::string& name,
                                                      const std::string& vertex_shader_source,
                                                      const std::string& fragment_shader_source)
{
    std::shared_ptr<Shader> shader = Shader::build(this, name, vertex_shader_source, fragment_shader_source);
    if (shader) {
        m_shaders.emplace_back(shader);
    }
    return shader;
}

void GraphicsContext::force_reloads()
{
    m_stencil_func  = StencilFunc::INVALID;
    m_stencil_mask  = 0;
    m_blend_mode    = BlendMode::INVALID;
    m_bound_texture = 0;
    m_bound_shader  = 0;
}

void GraphicsContext::_bind_texture(const GLuint texture_id)
{
    if (texture_id != m_bound_texture) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        m_bound_texture = texture_id;
    }
}

void GraphicsContext::_bind_shader(const GLuint shader_id)
{
    if (shader_id != m_bound_shader) {
        glUseProgram(shader_id);
        m_bound_shader = shader_id;
    }
}

} // namespace notf
