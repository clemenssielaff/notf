#include "graphics/graphics_system.hpp"

#include "app/glfw.hpp" // this should be the only time anything from "app" is referenced in the graphics module
#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/resource_manager.hpp"
#include "graphics/frame_buffer.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/opengl.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/shader.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/texture.hpp"

namespace {
NOTF_USING_NAMESPACE;

/// The GraphicsSystem is the first GraphicsContext to be initialized, but it derives from GraphicsContext.
/// Therefore, in order to have this method executed BEFORE the GraphicsContext constructor, it is injected into the
/// call.
valid_ptr<GLFWwindow*> load_gl_functions(valid_ptr<GLFWwindow*> window)
{
    struct TinyContextGuard {
        TinyContextGuard(GLFWwindow* window) { glfwMakeContextCurrent(window); }
        ~TinyContextGuard() { glfwMakeContextCurrent(nullptr); }
    } context_guard(window);

    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        NOTF_THROW(GraphicsContext::graphics_context_error, "gladLoadGLES2Loader failed");
    }
    return window;
}

} // namespace

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

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

TheGraphicsSystem::Extensions::Extensions()
{
    // initialize the members
    NOTF_CHECK_GL_EXTENSION(anisotropic_filter, GL_EXT_texture_filter_anisotropic);
    NOTF_CHECK_GL_EXTENSION(gpu_shader5, GL_EXT_gpu_shader5);
}

#undef NOTF_CHECK_GL_EXTENSION

// ================================================================================================================== //

TheGraphicsSystem::Environment::Environment()
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

std::atomic<bool> TheGraphicsSystem::s_is_running{false};

TheGraphicsSystem::TheGraphicsSystem(valid_ptr<GLFWwindow*> shared_window)
    : GraphicsContext(load_gl_functions(shared_window))
{
    s_is_running.store(true);
}

TheGraphicsSystem::~TheGraphicsSystem() { _shutdown(); }

const TheGraphicsSystem::Extensions& TheGraphicsSystem::get_extensions()
{
    static const Extensions singleton;
    return singleton;
}

const TheGraphicsSystem::Environment& TheGraphicsSystem::get_environment()
{
    static const Environment singleton;
    return singleton;
}

TexturePtr TheGraphicsSystem::get_texture(const TextureId& id) const
{
    auto it = m_textures.find(id);
    if (it == m_textures.end()) {
        NOTF_THROW(out_of_bounds, "The GraphicsSystem does not contain a Texture with ID \"{}\"", id);
    }
    return it->second.lock();
}

ShaderPtr TheGraphicsSystem::get_shader(const ShaderId& id) const
{
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        NOTF_THROW(out_of_bounds, "The GraphicsSystem does not contain a Shader with ID \"{}\"", id);
    }
    return it->second.lock();
}

PipelinePtr TheGraphicsSystem::get_pipeline(const PipelineId& id) const
{
    auto it = m_pipelines.find(id);
    if (it == m_pipelines.end()) {
        NOTF_THROW(out_of_bounds, "The GraphicsSystem does not contain a Pipeline with ID \"{}\"", id);
    }
    return it->second.lock();
}

FrameBufferPtr TheGraphicsSystem::get_framebuffer(const FrameBufferId& id) const
{
    auto it = m_framebuffers.find(id);
    if (it == m_framebuffers.end()) {
        NOTF_THROW(out_of_bounds, "The GraphicsSystem does not contain a FrameBuffer with ID \"{}\"", id);
    }
    return it->second.lock();
}

void TheGraphicsSystem::_post_initialization() { m_font_manager = FontManager::create(*this); }

void TheGraphicsSystem::_shutdown()
{
    // you can only close the application once
    if (!s_is_running.load()) {
        return;
    }
    s_is_running.store(false);

    // destroy the font manager
    m_font_manager.reset();

    // cleanup unused resources
    ResourceManager::get_instance().cleanup();

    // deallocate and invalidate all remaining Textures
    for (auto itr : m_textures) {
        if (TexturePtr texture = itr.second.lock()) {
            log_warning << "Deallocating live Texture: \"" << texture->get_name() << "\"";
            Texture::Access<TheGraphicsSystem>::deallocate(*texture);
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (auto itr : m_shaders) {
        if (ShaderPtr shader = itr.second.lock()) {
            log_warning << "Deallocating live Shader: \"" << shader->get_name() << "\"";
            Shader::Access<TheGraphicsSystem>::deallocate(*shader);
        }
    }
    m_shaders.clear();

    // deallocate and invalidate all remaining FrameBuffers
    for (auto itr : m_framebuffers) {
        if (FrameBufferPtr framebuffer = itr.second.lock()) {
            log_warning << "Deallocating live FrameBuffer: \"" << framebuffer->get_id() << "\"";
            FrameBuffer::Access<TheGraphicsSystem>::deallocate(*framebuffer);
        }
    }
    m_framebuffers.clear();

    // deallocate and invalidate all remaining Pipelines
    for (auto itr : m_pipelines) {
        if (PipelinePtr pipeline = itr.second.lock()) {
            log_warning << "Deallocating live Pipeline: \"" << pipeline->get_id() << "\"";
            Pipeline::Access<TheGraphicsSystem>::deallocate(*pipeline);
        }
    }
    m_framebuffers.clear();
}

void TheGraphicsSystem::_register_new(TexturePtr texture)
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

void TheGraphicsSystem::_register_new(ShaderPtr shader)
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

void TheGraphicsSystem::_register_new(FrameBufferPtr framebuffer)
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

void TheGraphicsSystem::_register_new(PipelinePtr pipeline)
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

void TheGraphicsSystem::release_shader_compiler() { notf_check_gl(glReleaseShaderCompiler()); }

NOTF_CLOSE_NAMESPACE
