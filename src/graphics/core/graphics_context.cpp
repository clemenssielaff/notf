#include "graphics/core/graphics_context.hpp"

#include <set>

#include "app/glfw.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/resource_manager.hpp"
#include "common/vector.hpp"
#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/text/font_manager.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

std::pair<GLenum, GLenum> convert_blend_mode(const BlendMode::Mode blend_mode)
{
    GLenum sfactor = 0;
    GLenum dfactor = 0;
    switch (blend_mode) {
    case (BlendMode::SOURCE_OVER):
        sfactor = GL_ONE;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::SOURCE_IN):
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_OUT):
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ZERO;
        break;
    case (BlendMode::SOURCE_ATOP):
        sfactor = GL_DST_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OVER):
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ONE;
        break;
    case (BlendMode::DESTINATION_IN):
        sfactor = GL_ZERO;
        dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_OUT):
        sfactor = GL_ZERO;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case (BlendMode::DESTINATION_ATOP):
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_SRC_ALPHA;
        break;
    case (BlendMode::LIGHTER):
        sfactor = GL_ONE;
        dfactor = GL_ONE;
        break;
    case (BlendMode::COPY):
        sfactor = GL_ONE;
        dfactor = GL_ZERO;
        break;
    case (BlendMode::XOR):
        sfactor = GL_ONE_MINUS_DST_ALPHA;
        dfactor = GL_ONE_MINUS_SRC_ALPHA;
        break;
    default:
        NOTF_ASSERT(false);
    }
    return {sfactor, dfactor};
}

} // namespace

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

#if NOTF_LOG_LEVEL >= NOTF_LOG_LEVEL_INFO
#define NOTF_CHECK_GL_EXTENSION(member, extension)                                                                 \
    member = GLAD_##extension;                                                                                     \
    if (member) {                                                                                                  \
        notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
                .input                                                                                             \
            << "Found OpenGL extension: \"" << #extension << "\"";                                                 \
    }                                                                                                              \
    else {                                                                                                         \
        notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
                .input                                                                                             \
            << "Could not find OpenGL extension: \"" << #extension << "\"";                                        \
    }
#else
#define CHECK_EXTENSION(member, name) member = NOTF_CONCAT(GLAD, extension)
#endif

GraphicsContext::Extensions::Extensions()
{
    // initialize the members
    NOTF_CHECK_GL_EXTENSION(anisotropic_filter, GL_EXT_texture_filter_anisotropic);
    NOTF_CHECK_GL_EXTENSION(nv_gpu_shader5, GL_EXT_gpu_shader5);
}

#undef NOTF_CHECK_GL_EXTENSION

// ================================================================================================================== //

GraphicsContext::Environment::Environment()
{
    constexpr GLint reserved_texture_slots = 1;

    { // max render buffer size
        GLint max_renderbuffer_size = -1;
        notf_check_gl(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size));
        max_render_buffer_size = narrow_cast<decltype(max_render_buffer_size)>(max_renderbuffer_size);
    }

    { // color attachment count
        GLint max_color_attachments = -1;
        notf_check_gl(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments));
        color_attachment_count = narrow_cast<decltype(color_attachment_count)>(max_color_attachments);
    }

    { // texture slot count
        GLint max_image_units = -1;
        notf_check_gl(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_image_units));
        texture_slot_count = narrow_cast<decltype(texture_slot_count)>(max_image_units - reserved_texture_slots);
    }

    { // font atlas texture slot
        font_atlas_texture_slot = texture_slot_count;
    }
}

// ================================================================================================================== //

GraphicsContext::GraphicsContext(GLFWwindow* window) : m_window(window)
{
    if (!window) {
        NOTF_THROW(runtime_error, "Failed to create a new GraphicsContext without a window (given pointer is null).");
    }

    { // GLFW defaults
        CurrentGuard guard = make_current();
        gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
        glfwSwapInterval(m_has_vsync ? 1 : 0);

        { // sanity check (otherwise OpenGL may happily return `null` on all calls until something crashes later)
            const GLubyte* version;
            notf_check_gl(version = glGetString(GL_VERSION));
            if (!version) {
                NOTF_THROW(runtime_error, "Failed to create an OpenGL context");
            }
        }

        // OpenGL hints
        notf_check_gl(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
        notf_check_gl(glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE));

        // apply the default state
        m_state = _create_state();
        set_stencil_mask(m_state.stencil_mask, /* force = */ true);
        set_blend_mode(m_state.blend_mode, /* force = */ true);
        clear(m_state.clear_color, Buffer::COLOR);

        // create auxiliary objects
        m_font_manager = FontManager::create(*this);
    }
}

GraphicsContext::~GraphicsContext()
{
    // release all resources that are bound by the context
    m_font_manager.reset();
    _unbind_all_textures();
    _unbind_pipeline();
    _unbind_framebuffer();

    // cleanup unused resources
    ResourceManager::get_instance().cleanup();

    // deallocate and invalidate all remaining Textures
    for (auto itr : m_textures) {
        if (TexturePtr texture = itr.second.lock()) {
            log_warning << "Deallocating live Texture: \"" << texture->get_name() << "\"";
            Texture::Access<GraphicsContext>::deallocate(*texture);
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (auto itr : m_shaders) {
        if (ShaderPtr shader = itr.second.lock()) {
            log_warning << "Deallocating live Shader: \"" << shader->get_name() << "\"";
            Shader::Access<GraphicsContext>::deallocate(*shader);
        }
    }
    m_shaders.clear();

    // deallocate and invalidate all remaining FrameBuffers
    for (auto itr : m_framebuffers) {
        if (FrameBufferPtr framebuffer = itr.second.lock()) {
            log_warning << "Deallocating live FrameBuffer: \"" << framebuffer->get_id() << "\"";
            FrameBuffer::Access<GraphicsContext>::deallocate(*framebuffer);
        }
    }
    m_framebuffers.clear();

    // deallocate and invalidate all remaining Pipelines
    for (auto itr : m_pipelines) {
        if (PipelinePtr pipeline = itr.second.lock()) {
            log_warning << "Deallocating live Pipeline: \"" << pipeline->get_id() << "\"";
            Pipeline::Access<GraphicsContext>::deallocate(*pipeline);
        }
    }
    m_framebuffers.clear();
}

const GraphicsContext::Extensions& GraphicsContext::get_extensions()
{
    static const Extensions singleton;
    return singleton;
}

const GraphicsContext::Environment& GraphicsContext::get_environment()
{
    static const Environment singleton;
    return singleton;
}

Size2i GraphicsContext::get_window_size() const
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
    notf_check_gl(glStencilMask(mask));
}

void GraphicsContext::set_blend_mode(const BlendMode mode, const bool force)
{
    if (mode == m_state.blend_mode && !force) {
        return;
    }
    m_state.blend_mode = mode;

    /// rgb
    GLenum rgb_sfactor, rgb_dfactor;
    std::tie(rgb_sfactor, rgb_dfactor) = convert_blend_mode(mode.rgb);

    // alpha
    GLenum alpha_sfactor, alpha_dfactor;
    std::tie(alpha_sfactor, alpha_dfactor) = convert_blend_mode(mode.alpha);

    notf_check_gl(glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor));
}

void GraphicsContext::set_render_area(Aabri area, const bool force)
{
    if (!area.is_valid()) {
        NOTF_THROW(runtime_error, "Cannot set to an invalid render area");
    }
    if (area != m_state.render_area || force) {
        notf_check_gl(glViewport(area.get_left(), area.get_bottom(), area.get_width(), area.get_height()));
        m_state.render_area = std::move(area);
    }
}

void GraphicsContext::clear(Color color, const BufferFlags buffers)
{
    if (color != m_state.clear_color) {
        m_state.clear_color = std::move(color);
        notf_check_gl(
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
    notf_check_gl(glClear(gl_flags));
}

// TODO: access control for begin_ and end_frame
void GraphicsContext::begin_frame() { clear(m_state.clear_color, Buffer::COLOR | Buffer::DEPTH | Buffer::STENCIL); }

void GraphicsContext::finish_frame() { glfwSwapBuffers(m_window); }

TexturePtr GraphicsContext::get_texture(const TextureId& id) const
{
    auto it = m_textures.find(id);
    if (it == m_textures.end()) {
        NOTF_THROW(out_of_bounds, "GraphicsContext does not contain a Texture with ID \"{}\"", id);
    }
    return it->second.lock();
}

void GraphicsContext::bind_texture(const Texture* texture, uint slot)
{
    if (!texture) {
        return unbind_texture(slot);
    }

    if (slot >= get_environment().texture_slot_count) {
        NOTF_THROW(runtime_error, "Invalid texture slot: {} - largest texture slot is: {}", slot,
                   get_environment().texture_slot_count - 1);
    }

    if (texture == m_state.texture_slots[slot].get()) {
        return;
    }

    if (!texture->is_valid()) {
        NOTF_THROW(runtime_error, "Cannot bind invalid texture \"{}\"", texture->get_name());
    }

    notf_check_gl(glActiveTexture(GL_TEXTURE0 + slot));
    notf_check_gl(glBindTexture(GL_TEXTURE_2D, texture->get_id().value()));

    m_state.texture_slots[slot] = texture->shared_from_this();
}

void GraphicsContext::unbind_texture(uint slot)
{
    if (slot >= get_environment().texture_slot_count) {
        NOTF_THROW(runtime_error, "Invalid texture slot: {} - largest texture slot is: {}", slot,
                   get_environment().texture_slot_count - 1);
    }

    if (m_state.texture_slots.at(slot) == nullptr) {
        return;
    }

    notf_check_gl(glActiveTexture(GL_TEXTURE0 + slot));
    notf_check_gl(glBindTexture(GL_TEXTURE_2D, 0));

    m_state.texture_slots[slot].reset();
}

ShaderPtr GraphicsContext::get_shader(const ShaderId& id) const
{
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        NOTF_THROW(out_of_bounds, "GraphicsContext does not contain a Shader with ID \"{}\"", id);
    }
    return it->second.lock();
}

GraphicsContext::PipelineGuard GraphicsContext::bind_pipeline(const PipelinePtr& pipeline)
{
    _bind_pipeline(pipeline);
    return PipelineGuard(*this, pipeline);
}

FrameBufferPtr GraphicsContext::get_framebuffer(const FrameBufferId& id) const
{
    auto it = m_framebuffers.find(id);
    if (it == m_framebuffers.end()) {
        NOTF_THROW(out_of_bounds, "GraphicsContext does not contain a FrameBuffer with ID \"{}\"", id);
    }
    return it->second.lock();
}

GraphicsContext::FramebufferGuard GraphicsContext::bind_framebuffer(const FrameBufferPtr& framebuffer)
{
    _bind_framebuffer(framebuffer);
    return FramebufferGuard(*this, framebuffer);
}

std::unique_lock<RecursiveMutex> GraphicsContext::_make_current()
{
    auto lock = std::unique_lock<RecursiveMutex>(m_mutex);
    if (m_recursion_counter++ == 0) {
        glfwMakeContextCurrent(m_window);
    }
    return lock;
}

void GraphicsContext::_release_current()
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (--m_recursion_counter == 0) {
        glfwMakeContextCurrent(nullptr);
    }
}

void GraphicsContext::_unbind_all_textures()
{
    for (uint slot = 0; slot < get_environment().texture_slot_count; ++slot) {
        unbind_texture(slot);
    }
}

GraphicsContext::State GraphicsContext::_create_state() const
{
    State result; // default constructed

    // query number of texture slots
    result.texture_slots.resize(get_environment().texture_slot_count);

    // query current window size
    result.render_area = Aabri(get_window_size());

    return result;
}

void GraphicsContext::_bind_pipeline(const PipelinePtr& pipeline)
{
    if (!pipeline) {
        _unbind_pipeline();
    }
    else if (pipeline != m_state.pipeline) {
        notf_check_gl(glUseProgram(0));
        notf_check_gl(glBindProgramPipeline(pipeline->get_id().value()));
        m_state.pipeline = pipeline;
    }
}

void GraphicsContext::_unbind_pipeline(const PipelinePtr& pipeline)
{
    if (pipeline && pipeline != m_state.pipeline) {
        log_critical << "Did not find expected Pipeline \"" << pipeline->get_id() << "\" to unbind, ignoring";
        return;
    }
    if (m_state.pipeline) {
        notf_check_gl(glUseProgram(0));
        notf_check_gl(glBindProgramPipeline(0));
        m_state.pipeline.reset();
    }
}

void GraphicsContext::_bind_framebuffer(const FrameBufferPtr& framebuffer)
{
    if (!framebuffer) {
        _unbind_framebuffer();
    }
    else if (framebuffer != m_state.framebuffer) {
        notf_check_gl(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->get_id().value()));
        m_state.framebuffer = framebuffer;
    }
}

void GraphicsContext::_unbind_framebuffer(const FrameBufferPtr& framebuffer)
{
    if (framebuffer && framebuffer != m_state.framebuffer) {
        log_critical << "Did not find expected FrameBuffer \"" << framebuffer->get_id() << "\" to unbind, ignoring";
        return;
    }
    if (m_state.framebuffer) {
        notf_check_gl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        m_state.framebuffer.reset();
    }
}

void GraphicsContext::_register_new(TexturePtr texture)
{
    auto it = m_textures.find(texture->get_id());
    if (it == m_textures.end()) {
        m_textures.emplace(texture->get_id(), texture); // insert new
    }
    else if (it->second.expired()) {
        it->second = texture; // update expired
    }
    else {
        NOTF_THROW(internal_error, "Failed to register a new Texture with the same ID as an existing Texture: \"{}\"",
                   texture->get_id());
    }
}

void GraphicsContext::_register_new(ShaderPtr shader)
{
    auto it = m_shaders.find(shader->get_id());
    if (it == m_shaders.end()) {
        m_shaders.emplace(shader->get_id(), shader); // insert new
    }
    else if (it->second.expired()) {
        it->second = shader; // update expired
    }
    else {
        NOTF_THROW(internal_error, "Failed to register a new Shader with the same ID as an existing Shader: \"{}\"",
                   shader->get_id());
    }
}

void GraphicsContext::_register_new(FrameBufferPtr framebuffer)
{
    auto it = m_framebuffers.find(framebuffer->get_id());
    if (it == m_framebuffers.end()) {
        m_framebuffers.emplace(framebuffer->get_id(), framebuffer); // insert new
    }
    else if (it->second.expired()) {
        it->second = framebuffer; // update expired
    }
    else {
        NOTF_THROW(internal_error,
                   "Failed to register a new Framebuffer with the same ID as an existing Framebuffer: \"{}\"",
                   framebuffer->get_id());
    }
}

void GraphicsContext::_register_new(PipelinePtr pipeline)
{
    auto it = m_pipelines.find(pipeline->get_id());
    if (it == m_pipelines.end()) {
        m_pipelines.emplace(pipeline->get_id(), pipeline); // insert new
    }
    else if (it->second.expired()) {
        it->second = pipeline; // update expired
    }
    else {
        NOTF_THROW(internal_error, "Failed to register a new Pipeline with the same ID as an existing Pipeline: \"{}\"",
                   pipeline->get_id());
    }
}

void GraphicsContext::release_shader_compiler() { notf_check_gl(glReleaseShaderCompiler()); }

NOTF_CLOSE_NAMESPACE

/* Something to think of, courtesy of the OpenGL ES book:
 * What happens if we are rendering into a texture and at the same time use this texture object as a texture in a
 * fragment shader? Will the OpenGL ES implementation generate an error when such a situation arises? In some cases, it
 * is possible for the OpenGL ES implementation to determine if a texture object is being used as a texture input and a
 * framebuffer attachment into which we are currently drawing. glDrawArrays and glDrawElements could then generate an
 * error. To ensure that glDrawArrays and glDrawElements can be executed as rapidly as possible, however, these checks
 * are not performed. Instead of generating an error, in this case rendering results are undefined. It is the
 * application’s responsibility to make sure that this situation does not occur.
 */
