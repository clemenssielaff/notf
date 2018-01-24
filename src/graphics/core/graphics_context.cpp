#include "graphics/core/graphics_context.hpp"

#include <cassert>
#include <cstring>
#include <set>

#include "app/core/glfw.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/text/font_manager.hpp"
#include "utils/narrow_cast.hpp"

//====================================================================================================================//

namespace notf {

#if NOTF_LOG_LEVEL >= NOTF_LOG_LEVEL_INFO
#define CHECK_EXTENSION(member, name)                                                                              \
    member = extensions.count(name);                                                                               \
    if (member) {                                                                                                  \
        notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
                .input                                                                                             \
            << "Found OpenGL extension:\"" << name << "\"";                                                        \
    }                                                                                                              \
    else {                                                                                                         \
        notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
                .input                                                                                             \
            << "Could not find OpenGL extension:\"" << name << "\"";                                               \
    }
#else
#define CHECK_EXTENSION(member, name) member = extensions.count(name);
#endif

GraphicsContext::Extensions::Extensions()
{
    std::set<std::string> extensions;
    { // create a set of all available extensions
        GLint extension_count;
        gl_check(glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count));
        for (GLint i = 0; i < extension_count; ++i) {
            //  log_trace << "Found OpenGL extension: " << glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
            extensions.emplace(
                std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)))));
        }
    }

    // initialize the members
    CHECK_EXTENSION(anisotropic_filter, "GL_EXT_texture_filter_anisotropic");
    CHECK_EXTENSION(nv_gpu_shader5, "GL_NV_gpu_shader5");
}

#undef CHECK_EXTENSION

//====================================================================================================================//

GraphicsContext::Environment::Environment()
{
    constexpr GLint reserved_texture_slots = 1;

    { // max render buffer size
        GLint max_renderbuffer_size = -1;
        gl_check(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size));
        max_render_buffer_size = narrow_cast<decltype(max_render_buffer_size)>(max_renderbuffer_size);
    }

    { // color attachment count
        GLint max_color_attachments = -1;
        gl_check(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments));
        color_attachment_count = narrow_cast<decltype(color_attachment_count)>(max_color_attachments);
    }

    { // texture slot count
        GLint max_image_units = -1;
        gl_check(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_image_units));
        texture_slot_count = narrow_cast<decltype(texture_slot_count)>(max_image_units - reserved_texture_slots);
    }

    { // font atlas texture slot
        font_atlas_texture_slot = texture_slot_count;
    }
}

//====================================================================================================================//

GraphicsContext::GraphicsContext(GLFWwindow* window)
    : m_window(window), m_state(), m_has_vsync(true), m_textures(), m_shaders(), m_font_manager()
{
    if (!window) {
        throw_runtime_error("Failed to create a new GraphicsContext without a window (given pointer is null).");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_has_vsync ? 1 : 0);

    gl_check(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
    gl_check(glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE));

    m_state = create_state();

    m_font_manager = std::make_unique<FontManager>(*this);
}

GraphicsContext::~GraphicsContext()
{
    // delete the Font Manager before warning about deallocating live textures
    m_font_manager.reset();

    // deallocate and invalidate all remaining Textures
    for (auto itr : m_textures) {
        std::weak_ptr<Texture>& texture_weakptr = itr.second;
        std::shared_ptr<Texture> texture        = texture_weakptr.lock();
        if (texture) {
            log_warning << "Deallocating live Texture: \"" << texture->name() << "\"";
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (auto itr : m_shaders) {
        std::weak_ptr<Shader>& shader_weakptr = itr.second;
        std::shared_ptr<Shader> shader        = shader_weakptr.lock();
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

void GraphicsContext::set_stencil_mask(const GLuint mask)
{
    if (mask != m_state.stencil_mask) {
        m_state.stencil_mask = mask;
        gl_check(glStencilMask(mask));
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
    gl_check(glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor));
}

void GraphicsContext::bind_texture(Texture* texture, uint slot)
{
    if (!texture) {
        return unbind_texture(slot);
    }

    if (slot >= environment().texture_slot_count) {
        std::stringstream ss;
        ss << "Invalid texture slot: " << slot << " - largest texture slot is:" << environment().texture_slot_count - 1;
        throw_runtime_error(ss.str());
    }

    if (texture == m_state.texture_slots[slot].get()) {
        return;
    }

    if (!texture->is_valid()) {
        throw_runtime_error(string_format("Cannot bind invalid texture \"%s\"", texture->name().c_str()));
    }

    gl_check(glActiveTexture(GL_TEXTURE0 + slot));
    gl_check(glBindTexture(GL_TEXTURE_2D, texture->id()));

    m_state.texture_slots[slot] = texture->shared_from_this();
}

void GraphicsContext::unbind_texture(uint slot)
{
    if (slot >= environment().texture_slot_count) {
        std::stringstream ss;
        ss << "Invalid texture slot: " << slot << " - largest texture slot is:" << environment().texture_slot_count - 1;
        throw_runtime_error(ss.str());
    }

    if (m_state.texture_slots.at(slot) == nullptr) {
        return;
    }

    gl_check(glActiveTexture(GL_TEXTURE0 + slot));
    gl_check(glBindTexture(GL_TEXTURE_2D, 0));

    m_state.texture_slots[slot].reset();
}

void GraphicsContext::unbind_all_textures()
{
    for (uint slot = 0; slot < environment().texture_slot_count; ++slot) {
        unbind_texture(slot);
    }
}

void GraphicsContext::bind_pipeline(PipelinePtr& pipeline)
{
    if (!pipeline) {
        return unbind_pipeline();
    }

    if (pipeline != m_state.pipeline) {
        gl_check(glUseProgram(0));
        gl_check(glBindProgramPipeline(pipeline->id()));

        m_state.pipeline = pipeline;
    }
}

void GraphicsContext::unbind_pipeline()
{
    if (m_state.pipeline) {
        gl_check(glUseProgram(0));
        gl_check(glBindProgramPipeline(0));

        m_state.pipeline.reset();
    }
}

void GraphicsContext::bind_framebuffer(FrameBufferPtr& framebuffer)
{
    if (!framebuffer) {
        return unbind_framebuffer();
    }
    if (framebuffer != m_state.framebuffer) {
        gl_check(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->id()));
        m_state.framebuffer = framebuffer;
    }
}

void GraphicsContext::unbind_framebuffer()
{
    if (m_state.framebuffer) {
        gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        m_state.framebuffer.reset();
    }
}

void GraphicsContext::_release_shader_compiler() { gl_check(glReleaseShaderCompiler()); }

} // namespace notf

/* Something to think of, courtesy of the OpenGL ES book:
 * What happens if we are rendering into a texture and at the same time use this texture object as a texture in a
 * fragment shader? Will the OpenGL ES implementation generate an error when such a situation arises? In some cases, it
 * is possible for the OpenGL ES implementation to determine if a texture object is being used as a texture input and a
 * framebuffer attachment into which we are currently drawing. glDrawArrays and glDrawElements could then generate an
 * error. To ensure that glDrawArrays and glDrawElements can be executed as rapidly as possible, however, these checks
 * are not performed. Instead of generating an error, in this case rendering results are undefined. It is the
 * application’s responsibility to make sure that this situation does not occur.
 */
