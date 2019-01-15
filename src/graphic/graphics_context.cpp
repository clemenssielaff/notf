#include "notf/graphic/graphics_context.hpp"

#include <set>

#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"

#include "notf/app/graph/window.hpp"

#include "notf/graphic/frame_buffer.hpp"
#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/glfw.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/shader.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/uniform_buffer.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

std::pair<GLenum, GLenum> convert_blend_mode(const BlendMode::Mode blend_mode) {
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
    }
    return {sfactor, dfactor};
}

} // namespace

NOTF_OPEN_NAMESPACE

// vao guard ======================================================================================================== //

GraphicsContext::VaoGuard::VaoGuard(GLuint vao) : m_vao(vao) { NOTF_CHECK_GL(glBindVertexArray(m_vao)); }

GraphicsContext::VaoGuard::~VaoGuard() { NOTF_CHECK_GL(glBindVertexArray(0)); }

// graphics context ================================================================================================= //

GraphicsContext::GraphicsContext(valid_ptr<GLFWwindow*> window) : m_window(window) {
    ContextGuard guard(*this);

    { // sanity check (otherwise OpenGL may happily return `null` on all calls until something crashes later)
        const GLubyte* version;
        NOTF_CHECK_GL(version = glGetString(GL_VERSION));
        if (!version) { NOTF_THROW(OpenGLError, "Failed to create an OpenGL context"); }
    }

    // GLFW hints
    glfwSwapInterval(-1); // -1 in case EXT_swap_control_tear is supported

    // OpenGL hints
    NOTF_CHECK_GL(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
    NOTF_CHECK_GL(glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE));
    NOTF_CHECK_GL(glDisable(GL_DITHER));

    _reset_state();
}

GraphicsContext::~GraphicsContext() {
    NOTF_GUARD(make_current());
    m_state = {}; // delete the state with a current OpenGL context
}

GraphicsContext& GraphicsContext::get() {
    if (GLFWwindow* glfw_window = glfwGetCurrentContext()) {
        if (void* user_pointer = glfwGetWindowUserPointer(glfw_window)) {
            return static_cast<Window*>(user_pointer)->get_graphics_context();
        }
    }
    return *TheGraphicsSystem();
}

GraphicsContext::ContextGuard GraphicsContext::make_current() {
    GLFWwindow* glfw_window = glfwGetCurrentContext();
    if (glfw_window && glfw_window != m_window) {
        NOTF_THROW(OpenGLError,
                   "Cannot make a GraphicsContext current on this thread while another one is already current");
    }

    return ContextGuard(*this);
}

Size2i GraphicsContext::get_window_size() const {
    NOTF_ASSERT(is_current());
    Size2i result;
    glfwGetFramebufferSize(_get_window(), &result[0], &result[1]);
    return result;
}

void GraphicsContext::set_stencil_mask(const GLuint mask, const bool force) {
    if (mask == m_state.stencil_mask && !force) { return; }
    NOTF_ASSERT(is_current());
    m_state.stencil_mask = mask;
    NOTF_CHECK_GL(glStencilMask(mask));
}

void GraphicsContext::set_blend_mode(const BlendMode mode, const bool force) {
    if (mode == m_state.blend_mode && !force) { return; }
    NOTF_ASSERT(is_current());
    m_state.blend_mode = mode;

    /// rgb
    GLenum rgb_sfactor, rgb_dfactor;
    std::tie(rgb_sfactor, rgb_dfactor) = convert_blend_mode(mode.rgb);

    // alpha
    GLenum alpha_sfactor, alpha_dfactor;
    std::tie(alpha_sfactor, alpha_dfactor) = convert_blend_mode(mode.alpha);

    NOTF_CHECK_GL(glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor));
}

void GraphicsContext::set_render_area(Aabri area, const bool force) {
    if (!area.is_valid()) { NOTF_THROW(ValueError, "Cannot set to an invalid render area"); }
    if (area != m_state.render_area || force) {
        NOTF_ASSERT(is_current());
        NOTF_CHECK_GL(glViewport(area.left(), area.bottom(), area.get_width(), area.get_height()));
        m_state.render_area = std::move(area);
    }
}

void GraphicsContext::clear(Color color, const BufferFlags buffers) {
    NOTF_ASSERT(is_current());
    if (color != m_state.clear_color) {
        m_state.clear_color = std::move(color);
        NOTF_CHECK_GL(
            glClearColor(m_state.clear_color.r, m_state.clear_color.g, m_state.clear_color.b, m_state.clear_color.a));
    }

    GLenum gl_flags = 0;
    if (buffers & Buffer::COLOR) { gl_flags |= GL_COLOR_BUFFER_BIT; }
    if (buffers & Buffer::DEPTH) { gl_flags |= GL_DEPTH_BUFFER_BIT; }
    if (buffers & Buffer::STENCIL) { gl_flags |= GL_STENCIL_BUFFER_BIT; }
    NOTF_CHECK_GL(glClear(gl_flags));
}

// TODO: access control for begin_ and end_frame
void GraphicsContext::begin_frame() { clear(m_state.clear_color, Buffer::COLOR | Buffer::DEPTH | Buffer::STENCIL); }

void GraphicsContext::finish_frame() {
    NOTF_ASSERT(is_current());
    glfwSwapBuffers(_get_window());
}

void GraphicsContext::bind_texture(const Texture* texture, uint slot) {
    if (!texture) { return unbind_texture(slot); }

    const auto& environment = TheGraphicsSystem::get_environment();
    if (slot >= environment.texture_slot_count) {
        NOTF_THROW(OpenGLError, "Invalid texture slot: {} - largest texture slot is: {}", slot,
                   environment.texture_slot_count - 1);
    }

    if (texture == m_state.texture_slots[slot].get()) { return; }

    if (!texture->is_valid()) { NOTF_THROW(OpenGLError, "Cannot bind invalid texture \"{}\"", texture->get_name()); }

    NOTF_ASSERT(is_current());
    NOTF_CHECK_GL(glActiveTexture(GL_TEXTURE0 + slot));
    NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, texture->get_id().value()));

    m_state.texture_slots[slot] = texture->shared_from_this();
}

void GraphicsContext::unbind_texture(uint slot) {
    const auto& environment = TheGraphicsSystem::get_environment();
    if (slot >= environment.texture_slot_count) {
        NOTF_THROW(OpenGLError, "Invalid texture slot: {} - largest texture slot is: {}", slot,
                   environment.texture_slot_count - 1);
    }

    if (m_state.texture_slots.at(slot) == nullptr) { return; }

    NOTF_ASSERT(is_current());
    NOTF_CHECK_GL(glActiveTexture(GL_TEXTURE0 + slot));
    NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));

    m_state.texture_slots[slot].reset();
}

void GraphicsContext::bind_uniform_buffer(const uint slot, const AnyUniformBufferPtr& uniform_buffer,
                                          const size_t index) {
    if (slot >= m_state.uniform_buffers.size()) {
        NOTF_THROW(OpenGLError, "Cannot bind UniformBuffer \"{}\" to slot {} as the system only provides {}",
                   uniform_buffer->get_name(), slot, m_state.uniform_buffers.size());
    }

    if (!uniform_buffer) {
        unbind_uniform_buffer(slot);
        return;
    }

    UniformBinding& target = m_state.uniform_buffers[slot];
    if (uniform_buffer == target.buffer && index == target.index) {
        return; // noop
    }

    if (index >= uniform_buffer->get_block_count()) {
        NOTF_THROW(IndexError,
                   "Cannot bind element at index {} of UniformBuffer \"{}\", because it only has {} elements", index,
                   uniform_buffer->get_name(), uniform_buffer->get_block_count());
    }

    NOTF_ASSERT(is_current());
    const GLsizeiptr block_size = narrow_cast<GLsizeiptr>(uniform_buffer->get_block_size());
    const GLintptr buffer_offset = block_size * narrow_cast<GLsizei>(index);
    target = UniformBinding{uniform_buffer, index};
    NOTF_CHECK_GL(
        glBindBufferRange(GL_UNIFORM_BUFFER, slot, uniform_buffer->get_id().value(), buffer_offset, block_size));
}

void GraphicsContext::unbind_uniform_buffer(const uint slot, const AnyUniformBufferPtr& uniform_buffer) {
    if (slot >= m_state.uniform_buffers.size()) {
        NOTF_THROW(OpenGLError, "Cannot unbind UniformBuffer slot {} as the system only provides {}", slot,
                   m_state.uniform_buffers.size());
    }

    if (uniform_buffer && uniform_buffer != m_state.uniform_buffers[slot].buffer) {
        NOTF_LOG_WARN("Did not find expected UniformBuffer \"{}\" to unbind from slot {}", uniform_buffer->get_name(),
                      slot);
        return;
    }

    if (!m_state.uniform_buffers[slot].buffer) { return; }

    NOTF_ASSERT(is_current());
    m_state.uniform_buffers[slot] = {};
    NOTF_CHECK_GL(glBindBufferBase(GL_UNIFORM_BUFFER, slot, 0));
}

void GraphicsContext::_shutdown_once() {
    const auto current_guard = make_current();
    m_state = {};
}

std::unique_lock<RecursiveMutex> GraphicsContext::_make_current() {
    auto lock = std::unique_lock<RecursiveMutex>(m_mutex);
    if (m_recursion_counter++ == 0) { glfwMakeContextCurrent(m_window); }
    NOTF_ASSERT(glfwGetCurrentContext() == m_window);
    return lock;
}

void GraphicsContext::_release_current() {
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (--m_recursion_counter == 0) { glfwMakeContextCurrent(nullptr); }
}

void GraphicsContext::_unbind_all_textures() {
    const auto& environment = TheGraphicsSystem::get_environment();
    for (uint slot = 0; slot < environment.texture_slot_count; ++slot) {
        unbind_texture(slot);
    }
}

void GraphicsContext::_reset_state() {
    m_state = State(); // default constructed

    // apply information from the environment
    const auto& environment = TheGraphicsSystem::get_environment();
    m_state.texture_slots.resize(environment.texture_slot_count);
    m_state.uniform_buffers.resize(environment.uniform_buffer_count);

    // query current window size
    m_state.render_area = Aabri(get_window_size());

    // enforce the state (since we don't know the current OpenGL state)
    set_stencil_mask(m_state.stencil_mask, /* force = */ true);
    set_blend_mode(m_state.blend_mode, /* force = */ true);
    clear(m_state.clear_color, Buffer::COLOR);
}

void GraphicsContext::_bind_program(const ShaderProgramPtr& program) {
    if (!program) {
        _unbind_program();
    } else if (program != m_state.program) {
        NOTF_ASSERT(is_current());
        NOTF_CHECK_GL(glUseProgram(0));
        NOTF_CHECK_GL(glBindProgramPipeline(program->get_id().value()));
        m_state.program = program;
        ShaderProgram::AccessFor<GraphicsContext>::activate(*m_state.program.get());
    }
}

void GraphicsContext::_unbind_program(const ShaderProgramPtr& program) {
    if (program && program != m_state.program) {
        NOTF_LOG_CRIT("Did not find expected ShaderProgram \"{}\", ignoring call to unbind", program->get_id());
    } else if (m_state.program) {
        NOTF_ASSERT(is_current());
        NOTF_CHECK_GL(glUseProgram(0));
        NOTF_CHECK_GL(glBindProgramPipeline(0));
        ShaderProgram::AccessFor<GraphicsContext>::deactivate(*m_state.program.get());
        m_state.program.reset();
    }
}

void GraphicsContext::_bind_framebuffer(const FrameBufferPtr& framebuffer) {
    if (!framebuffer) {
        _unbind_framebuffer();
    } else if (framebuffer != m_state.framebuffer) {
        NOTF_ASSERT(is_current());
        NOTF_CHECK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->get_id().value()));
        m_state.framebuffer = framebuffer;
    }
}

void GraphicsContext::_unbind_framebuffer(const FrameBufferPtr& framebuffer) {
    if (framebuffer && framebuffer != m_state.framebuffer) {
        NOTF_LOG_CRIT("Did not find expected FrameBuffer \"{}\", ignoring call to unbind", framebuffer->get_id());
    } else if (m_state.framebuffer) {
        NOTF_ASSERT(is_current());
        NOTF_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        m_state.framebuffer.reset();
    }
}
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
