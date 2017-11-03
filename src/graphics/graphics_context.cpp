#include "graphics/graphics_context.hpp"

#include <assert.h>
#include <cstring>
#include <set>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/glfw.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "utils/narrow_cast.hpp"

namespace {

const uint ALL_ZEROS = 0x00000000;
const uint ALL_ONES  = 0xffffffff;

} // namespace

//====================================================================================================================//

namespace notf {

#if NOTF_LOG_LEVEL >= NOTF_LOG_LEVEL_INFO
#define CHECK_EXTENSION(member, name)                                                                              \
    member = extensions.count(name);                                                                               \
    if (member) {                                                                                                  \
        notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
                .input                                                                                             \
            << "Found OpenGL extension:\"" << name << "\"";                                                        \
    }
#else
#define CHECK_EXTENSION(member, name) member = extensions.count(name);
#endif

GraphicsContext::Extensions::Extensions()
{
    std::set<std::string> extensions;
    { // create a set of all available extensions
        GLint extension_count;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
        for (GLint i = 0; i < extension_count; ++i) {
            extensions.emplace(
                std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)))));
            extensions.emplace(
                std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)))));
            extensions.emplace(
                std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)))));
        }
        check_gl_error();
    }

    // initialize the members
    CHECK_EXTENSION(anisotropic_filter, "GL_EXT_texture_filter_anisotropic");
}

#undef CHECK_EXTENSION

//====================================================================================================================//

GraphicsContext::Environment::Environment()
{
    { // texture slots
        GLint _texture_slot_count = -1;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_texture_slot_count);
        check_gl_error();
        texture_slot_count = narrow_cast<decltype(texture_slot_count)>(_texture_slot_count);
    }
}

//====================================================================================================================//

GraphicsContext::GraphicsContext(GLFWwindow* window)
    : m_window(window), m_state(), m_has_vsync(true), m_textures(), m_shaders()
{
    if (!window) {
        throw_runtime_error("Failed to create a new GraphicsContext without a window (given pointer is null).");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_has_vsync ? 1 : 0);

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE);

    m_state = create_state();
}

GraphicsContext::~GraphicsContext()
{
    // deallocate and invalidate all remaining Textures
    for (std::weak_ptr<Texture> texture_weakptr : m_textures) {
        std::shared_ptr<Texture> texture = texture_weakptr.lock();
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

const GraphicsContext::Extensions& GraphicsContext::extensions()
{
    static const Extensions singleton;
    return singleton;
}

const GraphicsContext::Environment& GraphicsContext::environment()
{
    static const Environment singleton;
    return singleton;
}

GraphicsContext::State GraphicsContext::create_state() const
{
    State result;
    result.texture_slots.resize(environment().texture_slot_count);
    return result;
}

void GraphicsContext::set_vsync(const bool enabled)
{
    if (enabled != m_has_vsync) {
        m_has_vsync = enabled;
        glfwSwapInterval(m_has_vsync ? 1 : 0);
    }
}

void GraphicsContext::set_stencil_func(const StencilFunc func)
{
    if (func == m_state.stencil_func) {
        return;
    }
    m_state.stencil_func = func;

    switch (func) {
    case StencilFunc::ALWAYS:
        glStencilFunc(GL_ALWAYS, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::NEVER:
        glStencilFunc(GL_NEVER, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::LESS:
        glStencilFunc(GL_LESS, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::LEQUAL:
        glStencilFunc(GL_LEQUAL, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::GREATER:
        glStencilFunc(GL_GREATER, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::GEQUAL:
        glStencilFunc(GL_GEQUAL, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::EQUAL:
        glStencilFunc(GL_EQUAL, ALL_ZEROS, ALL_ONES);
        break;
    case StencilFunc::NOTEQUAL:
        glStencilFunc(GL_NOTEQUAL, ALL_ZEROS, ALL_ONES);
        break;
    }
    check_gl_error();
}

void GraphicsContext::set_stencil_mask(const GLuint mask)
{
    if (mask != m_state.stencil_mask) {
        m_state.stencil_mask = mask;
        glStencilMask(mask);
        check_gl_error();
    }
}

void GraphicsContext::set_blend_mode(const BlendMode mode)
{
    if (mode == m_state.blend_mode) {
        return;
    }
    m_state.blend_mode = mode;

    // color
    GLenum rgb_sfactor;
    GLenum rgb_dfactor;
    switch (mode.rgb) {
    case (BlendMode::SOURCE_OVER):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::SOURCE_IN):
        rgb_sfactor = GL_DST_ALPHA;
        rgb_dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_OUT):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_ATOP):
        rgb_sfactor = GL_DST_ALPHA;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OVER):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ONE;
        break;
    case (BlendMode::DESTINATION_IN):
        rgb_sfactor = GL_ZERO;
        rgb_dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OUT):
        rgb_sfactor = GL_ZERO;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_ATOP):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::LIGHTER):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ONE;
        break;
    case (BlendMode::COPY):
        rgb_sfactor = GL_ONE;
        rgb_dfactor = GL_ZERO;
        break;
    case (BlendMode::XOR):
        rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
        rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    default:
        rgb_sfactor = 0;
        rgb_dfactor = 0;
        assert(false);
        break;
    }

    // alpha
    GLenum alpha_sfactor;
    GLenum alpha_dfactor;
    switch (mode.alpha) {
    case (BlendMode::SOURCE_OVER):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::SOURCE_IN):
        alpha_sfactor = GL_DST_ALPHA;
        alpha_dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_OUT):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_ATOP):
        alpha_sfactor = GL_DST_ALPHA;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OVER):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ONE;
        break;
    case (BlendMode::DESTINATION_IN):
        alpha_sfactor = GL_ZERO;
        alpha_dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OUT):
        alpha_sfactor = GL_ZERO;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_ATOP):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::LIGHTER):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ONE;
        break;
    case (BlendMode::COPY):
        alpha_sfactor = GL_ONE;
        alpha_dfactor = GL_ZERO;
        break;
    case (BlendMode::XOR):
        alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
        alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    default:
        alpha_sfactor = 0;
        alpha_dfactor = 0;
        assert(false);
        break;
    }
    glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor);
    check_gl_error();
}

void GraphicsContext::bind_texture(TexturePtr texture, uint slot)
{
    if (!texture) {
        return unbind_texture(slot);
    }

    try {
        if (texture == m_state.texture_slots.at(slot)) {
            return;
        }
    }
    catch (std::out_of_range&) {
        std::stringstream ss;
        ss << "Invalid texture slot: " << slot << " - largest texture slot is:" << environment().texture_slot_count - 1;
        throw_runtime_error(ss.str());
    }

    if (!texture->is_valid()) {
        throw_runtime_error(string_format("Cannot bind invalid texture \"%s\"", texture->name().c_str()));
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture->id());
    check_gl_error();

    m_state.texture_slots[slot] = std::move(texture);
}

void GraphicsContext::unbind_texture(uint slot)
{
    try {
        if (m_state.texture_slots.at(slot) == nullptr) {
            return;
        }
    }
    catch (std::out_of_range&) {
        std::stringstream ss;
        ss << "Invalid texture slot: " << slot << " - largest texture slot is:" << environment().texture_slot_count - 1;
        throw_runtime_error(ss.str());
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
    check_gl_error();

    m_state.texture_slots[slot].reset();
}

void GraphicsContext::unbind_all_textures()
{
    for (uint slot = 0; slot < environment().texture_slot_count; ++slot) {
        unbind_texture(slot);
    }
}

void GraphicsContext::bind_shader(ShaderPtr shader)
{
    if (!shader) {
        return unbind_shader();
    }

    if (!shader->is_valid()) {
        throw_runtime_error(string_format("Cannot bind invalid shader \"%s\"", shader->name().c_str()));
    }

    if (shader != m_state.shader) {
        glUseProgram(shader->id());
        check_gl_error();

        m_state.shader = std::move(shader);
    }
}

void GraphicsContext::unbind_shader()
{
    if (m_state.shader) {
        glUseProgram(0);
        check_gl_error();

        m_state.shader.reset();
    }
}

void GraphicsContext::_release_shader_compiler()
{
    glReleaseShaderCompiler();
    check_gl_error();
}

} // namespace notf
