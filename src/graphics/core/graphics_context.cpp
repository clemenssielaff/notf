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
    : m_window(window), m_state(), m_has_vsync(true), m_textures(), m_shaders()
{
    if (!window) {
        notf_throw(runtime_error, "Failed to create a new GraphicsContext without a window (given pointer is null).");
    }

    // GLFW defaults
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_has_vsync ? 1 : 0);

    // OpenGL defaults
    gl_check(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
    gl_check(glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE));

    // apply the default state
    m_state = _create_state();
    set_stencil_mask(m_state.stencil_mask, /* force = */ true);
    set_blend_mode(m_state.blend_mode, /* force = */ true);
    clear(m_state.clear_color, Buffer::COLOR, /* force = */ true);
}

GraphicsContext::~GraphicsContext()
{
    // release all resources that are bound by the context
    unbind_all_textures();
    unbind_pipeline();
    unbind_framebuffer();

    // deallocate and invalidate all remaining Textures
    for (auto itr : m_textures) {
        if (TexturePtr texture = itr.second.lock()) {
            log_warning << "Deallocating live Texture: \"" << texture->name() << "\"";
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (auto itr : m_shaders) {
        if (ShaderPtr shader = itr.second.lock()) {
            log_warning << "Deallocating live Shader: \"" << shader->name() << "\"";
            shader->_deallocate();
        }
    }
    m_shaders.clear();

    // deallocate and invalidate all remaining FrameBuffers
    for (auto itr : m_framebuffers) {
        if (FrameBufferPtr framebuffer = itr.second.lock()) {
            log_warning << "Deallocating live FrameBuffer: \"" << framebuffer->id() << "\"";
            framebuffer->_deallocate();
        }
    }
    m_framebuffers.clear();
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

Size2i GraphicsContext::window_size() const
{
    Size2i result;
    glfwGetFramebufferSize(m_window, &result.width, &result.height);
    return result;
}

void GraphicsContext::set_vsync(const bool enabled)
{
    if (enabled != m_has_vsync) {
        m_has_vsync = enabled;
        glfwSwapInterval(m_has_vsync ? 1 : 0);
    }
}

void GraphicsContext::set_stencil_mask(const GLuint mask, const bool force)
{
    if (mask == m_state.stencil_mask && !force) {
        return;
    }
    m_state.stencil_mask = mask;
    gl_check(glStencilMask(mask));
}

void GraphicsContext::set_blend_mode(const BlendMode mode, const bool force)
{
    if (mode == m_state.blend_mode && !force) {
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

void GraphicsContext::set_render_size(Size2i buffer_size, const bool force)
{
    if (!buffer_size.is_valid()) {
        notf_throw(runtime_error, "Cannot set render size to invalid size");
    }
    if (buffer_size != m_state.render_size || force) {
        m_state.render_size = buffer_size;
        gl_check(glViewport(0, 0, buffer_size.width, buffer_size.height));
    }
}

void GraphicsContext::clear(Color color, const BufferFlags buffers, const bool force)
{
    if (color != m_state.clear_color || force) {
        m_state.clear_color = std::move(color);
        gl_check(
            glClearColor(m_state.clear_color.r, m_state.clear_color.g, m_state.clear_color.b, m_state.clear_color.a));
    }

    GLenum gl_flags = 0;
    if (buffers & Buffer::COLOR) {
        gl_flags |= GL_COLOR_BUFFER_BIT;
    }
    if (buffers & Buffer::DEPTH) {
        gl_flags |= GL_DEPTH_BUFFER_BIT;
    }
    if (buffers & Buffer::STENCIL) {
        gl_flags |= GL_STENCIL_BUFFER_BIT;
    }
    gl_check(glClear(gl_flags));
}

void GraphicsContext::begin_frame()
{
    clear(m_state.clear_color, Buffer::COLOR | Buffer::DEPTH | Buffer::STENCIL);
}

void GraphicsContext::finish_frame()
{
    glfwSwapBuffers(m_window);
}

TexturePtr GraphicsContext::texture(const TextureId& id) const
{
    auto it = m_textures.find(id);
    if (it == m_textures.end()) {
        notf_throw_format(out_of_range, "GraphicsContext does not contain a Texture with ID \"" << id << "\"");
    }
    return it->second.lock();
}

void GraphicsContext::bind_texture(const Texture* texture, uint slot)
{
    if (!texture) {
        return unbind_texture(slot);
    }

    if (slot >= environment().texture_slot_count) {
        notf_throw_format(runtime_error, "Invalid texture slot: " << slot << " - largest texture slot is:"
                                                                  << environment().texture_slot_count - 1);
    }

    if (texture == m_state.texture_slots[slot].get()) {
        return;
    }

    if (!texture->is_valid()) {
        notf_throw_format(runtime_error, "Cannot bind invalid texture \"" << texture->name() << "\"");
    }

    gl_check(glActiveTexture(GL_TEXTURE0 + slot));
    gl_check(glBindTexture(GL_TEXTURE_2D, texture->id().value()));

    m_state.texture_slots[slot] = texture->shared_from_this();
}

void GraphicsContext::unbind_texture(uint slot)
{
    if (slot >= environment().texture_slot_count) {
        notf_throw_format(runtime_error, "Invalid texture slot: " << slot << " - largest texture slot is:"
                                                                  << environment().texture_slot_count - 1);
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

ShaderPtr GraphicsContext::shader(const ShaderId& id) const
{
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        notf_throw_format(out_of_range, "GraphicsContext does not contain a Shader with ID \"" << id << "\"");
    }
    return it->second.lock();
}

void GraphicsContext::bind_pipeline(const PipelinePtr& pipeline)
{
    if (!pipeline) {
        return unbind_pipeline();
    }

    if (pipeline != m_state.pipeline) {
        gl_check(glUseProgram(0));
        gl_check(glBindProgramPipeline(pipeline->id().value()));

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

FrameBufferPtr GraphicsContext::framebuffer(const FrameBufferId& id) const
{
    auto it = m_framebuffers.find(id);
    if (it == m_framebuffers.end()) {
        notf_throw_format(out_of_range, "GraphicsContext does not contain a FrameBuffer with ID \"" << id << "\"");
    }
    return it->second.lock();
}

void GraphicsContext::bind_framebuffer(const FrameBufferPtr& framebuffer)
{
    if (!framebuffer) {
        return unbind_framebuffer();
    }
    if (framebuffer != m_state.framebuffer) {
        gl_check(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->id().value()));
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

GraphicsContext::State GraphicsContext::_create_state() const
{
    State result; // default constructed

    // query number of texture slots
    result.texture_slots.resize(environment().texture_slot_count);

    // query current window size
    result.render_size = window_size();

    return result;
}

void GraphicsContext::register_new(TexturePtr texture)
{
    auto it = m_textures.find(texture->id());
    if (it == m_textures.end()) {
        m_textures.emplace(texture->id(), texture); // insert new
    }
    else if (it->second.expired()) {
        it->second = texture; // update expired
    }
    else {
        notf_throw_format(internal_error, "Failed to register a new texture with the same ID as an existing "
                                          "texture: \""
                                              << texture->id() << "\"");
    }
}

void GraphicsContext::register_new(ShaderPtr shader)
{
    auto it = m_shaders.find(shader->id());
    if (it == m_shaders.end()) {
        m_shaders.emplace(shader->id(), shader); // insert new
    }
    else if (it->second.expired()) {
        it->second = shader; // update expired
    }
    else {
        notf_throw_format(internal_error, "Failed to register a new shader with the same ID as an existing shader: "
                                          "\"" << shader->id()
                                               << "\"");
    }
}

void GraphicsContext::register_new(FrameBufferPtr framebuffer)
{
    auto it = m_framebuffers.find(framebuffer->id());
    if (it == m_framebuffers.end()) {
        m_framebuffers.emplace(framebuffer->id(), framebuffer); // insert new
    }
    else if (it->second.expired()) {
        it->second = framebuffer; // update expired
    }
    else {
        notf_throw_format(internal_error, "Failed to register a new framebuffer with the same ID as an existing "
                                              << "framebuffer: \"" << framebuffer->id() << "\"");
    }
}

void GraphicsContext::release_shader_compiler() { gl_check(glReleaseShaderCompiler()); }

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
