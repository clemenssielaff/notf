#include "notf/graphic/graphics_system.hpp"

#include <algorithm>

#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/app/graph/window.hpp" // TODO: try to move graph includes out of graphics
#include "notf/app/resource_manager.hpp"

#include "notf/graphic/frame_buffer.hpp"
#include "notf/graphic/glfw.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/shader.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"

namespace {
NOTF_USING_NAMESPACE;

/// The GraphicsSystem owns the first GraphicsContext to be initialized.
/// In order to have this method executed BEFORE the GraphicsContext constructor, it is injected into the call.
valid_ptr<GLFWwindow*> load_gl_functions(valid_ptr<GLFWwindow*> window) {
    struct ContextGuard {
        ContextGuard(GLFWwindow* window) { glfwMakeContextCurrent(window); }
        ~ContextGuard() { glfwMakeContextCurrent(nullptr); }
    } guard(window);

    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        NOTF_THROW(OpenGLError, "gladLoadGLES2Loader failed");
    }
    return window;
}

} // namespace

NOTF_OPEN_NAMESPACE

namespace detail {

// extensions ======================================================================================================= //

#if NOTF_LOG_LEVEL <= 0
#define NOTF_CHECK_GL_EXTENSION(member, extension)                                                                 \
    member = GLAD_##extension;                                                                                     \
    if (member) {                                                                                                  \
        ::notf::TheLogger::get()->trace(fmt::format("Found OpenGL extension: \"{}\" ({}:{})", #extension,          \
                                                    ::notf::filename_from_path(__FILE__), __LINE__));              \
    } else {                                                                                                       \
        ::notf::TheLogger::get()->trace(fmt::format("Could not find OpenGL extension: \"{}\" ({}:{})", #extension, \
                                                    ::notf::filename_from_path(__FILE__), __LINE__));              \
    }
#else
#define CHECK_EXTENSION(member, name) member = NOTF_CONCAT(GLAD, extension)
#endif

GraphicsSystem::Extensions::Extensions() {
    // initialize the members
    NOTF_CHECK_GL_EXTENSION(anisotropic_filter, GL_EXT_texture_filter_anisotropic);
    NOTF_CHECK_GL_EXTENSION(gpu_shader5, GL_EXT_gpu_shader5);
}

#undef NOTF_CHECK_GL_EXTENSION

// environment ====================================================================================================== //

GraphicsSystem::Environment::Environment() {
    static constexpr GLint reserved_texture_slots = 1;

    { // max render buffer size
        GLint max_renderbuffer_size = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size));
        NOTF_ASSERT(max_renderbuffer_size >= 0);
        max_render_buffer_size = static_cast<decltype(max_render_buffer_size)>(max_renderbuffer_size);
    }

    { // color attachment count
        GLint max_color_attachments = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments));
        NOTF_ASSERT(max_color_attachments >= 0);
        color_attachment_count = static_cast<decltype(color_attachment_count)>(max_color_attachments);
    }

    { // texture slot count
        GLint max_combined_texture_image_units = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_combined_texture_image_units));
        NOTF_ASSERT(max_combined_texture_image_units >= 0);
        texture_slot_count
            = static_cast<decltype(texture_slot_count)>(max_combined_texture_image_units - reserved_texture_slots);
    }

    { // uniform buffer slot count
        GLint max_uniform_buffer_bindings = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_uniform_buffer_bindings));
        NOTF_ASSERT(max_uniform_buffer_bindings >= 0);
        uniform_slot_count = static_cast<decltype(uniform_slot_count)>(max_uniform_buffer_bindings);
    }

    { // vertex attribute count
        GLint max_vertex_attribs = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs));
        NOTF_ASSERT(max_vertex_attribs >= 0);
        vertex_attribute_count = static_cast<decltype(vertex_attribute_count)>(max_vertex_attribs);
    }

    { // vertex attribute count
        GLint max_samples = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_SAMPLES, &max_samples));
        NOTF_ASSERT(max_samples >= 0);
        if (max_samples > 0) {
            --max_samples; // in OpenGL "max" means one too many
        }
        max_sample_count = static_cast<decltype(max_sample_count)>(max_samples);
    }

    { // font atlas texture slot
        font_atlas_texture_slot = texture_slot_count;
    }
}

// graphics system ================================================================================================== //

const GraphicsSystem::Extensions& GraphicsSystem::_get_extensions() {
    static const Extensions singleton;
    return singleton;
}

const GraphicsSystem::Environment& GraphicsSystem::_get_environment() {
    static const Environment singleton;
    return singleton;
}

GraphicsSystem::GraphicsSystem(valid_ptr<GLFWwindow*> shared_window)
    : m_context(std::make_unique<GraphicsContext>(load_gl_functions(shared_window))) {}

GraphicsSystem::~GraphicsSystem() {

    NOTF_GUARD(m_context->make_current());

    // destroy the font manager
    m_font_manager.reset();

    // cleanup unused resources
    ResourceManager::get_instance().cleanup();

    // deallocate and invalidate all remaining Textures
    for (auto itr : m_textures) {
        if (TexturePtr texture = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live Texture \"{}\"", texture->get_name());
            Texture::AccessFor<GraphicsSystem>::deallocate(*texture);
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (auto itr : m_shaders) {
        if (AnyShaderPtr shader = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live Shader \"{}\"", shader->get_name());
            AnyShader::AccessFor<GraphicsSystem>::deallocate(*shader);
        }
    }
    m_shaders.clear();

    // deallocate and invalidate all remaining RenderBuffers
    for (auto itr : m_renderbuffers) {
        if (RenderBufferPtr renderbuffer = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live RenderBuffer \"{}\"", renderbuffer->get_name());
            RenderBuffer::AccessFor<GraphicsSystem>::deallocate(*renderbuffer);
        }
    }
    m_renderbuffers.clear();
}

GraphicsContext& GraphicsSystem::get_any_context() {
    if (GLFWwindow* glfw_window = glfwGetCurrentContext()) {
        if (void* user_pointer = glfwGetWindowUserPointer(glfw_window)) {
            return static_cast<Window*>(user_pointer)->get_graphics_context();
        }
    }
    return get_internal_context();
}

void GraphicsSystem::release_shader_compiler() { NOTF_CHECK_GL(glReleaseShaderCompiler()); }

void GraphicsSystem::_post_initialization() { m_font_manager = FontManager::create(); }

void GraphicsSystem::_register_new(TexturePtr texture) {
    auto it = m_textures.find(texture->get_id());
    if (it == m_textures.end()) {
        m_textures.emplace(texture->get_id(), texture); // insert new
    } else if (it->second.expired()) {
        it->second = texture; // update expired
    } else {
        NOTF_THROW(NotUniqueError, "Failed to register a new Texture with the same ID as an existing Texture: \"{}\"",
                   texture->get_id());
    }
}

void GraphicsSystem::_register_new(AnyShaderPtr shader) {
    auto it = m_shaders.find(shader->get_id());
    if (it == m_shaders.end()) {
        m_shaders.emplace(shader->get_id(), shader); // insert new
    } else if (it->second.expired()) {
        it->second = shader; // update expired
    } else {
        NOTF_THROW(NotUniqueError, "Failed to register a new Shader with the same ID as an existing Shader: \"{}\"",
                   shader->get_id());
    }
}

void GraphicsSystem::_register_new(RenderBufferPtr renderbuffer) {
    auto it = m_renderbuffers.find(renderbuffer->get_id());
    if (it == m_renderbuffers.end()) {
        m_renderbuffers.emplace(renderbuffer->get_id(), renderbuffer); // insert new
    } else if (it->second.expired()) {
        it->second = renderbuffer; // update expired
    } else {
        NOTF_THROW(NotUniqueError,
                   "Failed to register a new RenderBuffer with the same ID as an existing RenderBuffer: \"{}\"",
                   renderbuffer->get_id());
    }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
