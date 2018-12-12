#include "notf/graphic/graphics_system.hpp"

#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/frame_buffer.hpp"
#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/glfw.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/shader.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"

namespace {
NOTF_USING_NAMESPACE;

/// The GraphicsSystem is the first GraphicsContext to be initialized, but it derives from GraphicsContext.
/// Therefore, in order to have this method executed BEFORE the GraphicsContext constructor, it is injected into the
/// call.
valid_ptr<GLFWwindow*> load_gl_functions(valid_ptr<GLFWwindow*> window) {
    struct TinyContextGuard {
        TinyContextGuard(GLFWwindow* window) { glfwMakeContextCurrent(window); }
        ~TinyContextGuard() { glfwMakeContextCurrent(nullptr); }
    } context_guard(window);

    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        NOTF_THROW(OpenGLError, "gladLoadGLES2Loader failed");
    }
    return window;
}

} // namespace

NOTF_OPEN_NAMESPACE

namespace detail {

// ================================================================================================================== //

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

// ================================================================================================================== //

GraphicsSystem::Environment::Environment() {
    constexpr GLint reserved_texture_slots = 1;

    { // max render buffer size
        GLint max_renderbuffer_size = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size));
        max_render_buffer_size = narrow_cast<decltype(max_render_buffer_size)>(max_renderbuffer_size);
    }

    { // color attachment count
        GLint max_color_attachments = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments));
        color_attachment_count = narrow_cast<decltype(color_attachment_count)>(max_color_attachments);
    }

    { // texture slot count
        GLint max_image_units = -1;
        NOTF_CHECK_GL(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_image_units));
        texture_slot_count = narrow_cast<decltype(texture_slot_count)>(max_image_units - reserved_texture_slots);
    }

    { // font atlas texture slot
        font_atlas_texture_slot = texture_slot_count;
    }
}

// ================================================================================================================== //

const GraphicsSystem::Extensions& GraphicsSystem::_get_extensions() {
    static const Extensions singleton;
    return singleton;
}

const GraphicsSystem::Environment& GraphicsSystem::_get_environment() {
    static const Environment singleton;
    return singleton;
}

GraphicsSystem::GraphicsSystem(valid_ptr<GLFWwindow*> shared_window)
    : GraphicsContext(load_gl_functions(shared_window)) {}

GraphicsSystem::~GraphicsSystem() { _shutdown(); }

TexturePtr GraphicsSystem::get_texture(const TextureId& id) const {
    auto it = m_textures.find(id);
    if (it == m_textures.end()) {
        NOTF_THROW(OutOfBounds, "The GraphicsSystem does not contain a Texture with ID \"{}\"", id);
    }
    return it->second.lock();
}

ShaderPtr GraphicsSystem::get_shader(const ShaderId& id) const {
    auto it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        NOTF_THROW(OutOfBounds, "The GraphicsSystem does not contain a Shader with ID \"{}\"", id);
    }
    return it->second.lock();
}

ShaderProgramPtr GraphicsSystem::get_program(const ShaderProgramId id) const {
    auto it = m_programs.find(id);
    if (it == m_programs.end()) {
        NOTF_THROW(OutOfBounds, "The GraphicsSystem does not contain a ShaderProgram with ID \"{}\"", id);
    }
    return it->second.lock();
}

FrameBufferPtr GraphicsSystem::get_framebuffer(const FrameBufferId& id) const {
    auto it = m_framebuffers.find(id);
    if (it == m_framebuffers.end()) {
        NOTF_THROW(OutOfBounds, "The GraphicsSystem does not contain a FrameBuffer with ID \"{}\"", id);
    }
    return it->second.lock();
}

void GraphicsSystem::_post_initialization() { m_font_manager = FontManager::create(); }

void GraphicsSystem::_shutdown_once() {
    const auto current_guard = make_current();

    // shed the GraphicsContext state
    GraphicsContext::_shutdown_once();

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
        if (ShaderPtr shader = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live Shader \"{}\"", shader->get_name());
            Shader::AccessFor<GraphicsSystem>::deallocate(*shader);
        }
    }
    m_shaders.clear();

    // deallocate and invalidate all remaining FrameBuffers
    for (auto itr : m_framebuffers) {
        if (FrameBufferPtr framebuffer = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live FrameBuffer \"{}\"", framebuffer->get_id());
            FrameBuffer::AccessFor<GraphicsSystem>::deallocate(*framebuffer);
        }
    }
    m_framebuffers.clear();

    // deallocate and invalidate all remaining ShaderPrograms
    for (auto itr : m_programs) {
        if (ShaderProgramPtr program = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live ShaderProgram \"{}\"", program->get_id());
            ShaderProgram::AccessFor<GraphicsSystem>::deallocate(*program);
        }
    }
    m_framebuffers.clear();
}

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

void GraphicsSystem::_register_new(ShaderPtr shader) {
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

void GraphicsSystem::_register_new(FrameBufferPtr framebuffer) {
    auto it = m_framebuffers.find(framebuffer->get_id());
    if (it == m_framebuffers.end()) {
        m_framebuffers.emplace(framebuffer->get_id(), framebuffer); // insert new
    } else if (it->second.expired()) {
        it->second = framebuffer; // update expired
    } else {
        NOTF_THROW(NotUniqueError,
                   "Failed to register a new Framebuffer with the same ID as an existing Framebuffer: \"{}\"",
                   framebuffer->get_id());
    }
}

void GraphicsSystem::_register_new(ShaderProgramPtr program) {
    auto it = m_programs.find(program->get_id());
    if (it == m_programs.end()) {
        m_programs.emplace(program->get_id(), program); // insert new
    } else if (it->second.expired()) {
        it->second = program; // update expired
    } else {
        NOTF_THROW(NotUniqueError,
                   "Failed to register a new ShaderProgram with the same ID as an existing ShaderProgram: \"{}\"",
                   program->get_id());
    }
}

void GraphicsSystem::release_shader_compiler() { NOTF_CHECK_GL(glReleaseShaderCompiler()); }

} // namespace detail

NOTF_CLOSE_NAMESPACE
