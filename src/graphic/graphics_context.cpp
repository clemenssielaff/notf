#include "notf/graphic/graphics_context.hpp"

#include <set>

#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"

#include "notf/app/graph/window.hpp"

#include "notf/graphic/frame_buffer.hpp"
#include "notf/graphic/glfw.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/shader.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/uniform_buffer.hpp"
#include "notf/graphic/vertex_object.hpp"

NOTF_USING_NAMESPACE;

// graphics context guard =========================================================================================== //

GraphicsContext::Guard::Guard(GraphicsContext& context, std::unique_lock<RecursiveMutex>&& lock)
    : m_context(&context), m_mutex_lock(std::move(lock)) {}

GraphicsContext::Guard::~Guard() {
    if (!m_context) { return; }

    NOTF_ASSERT(m_mutex_lock.owns_lock());
    m_mutex_lock.unlock();

    const RecursiveMutex* mutex = m_mutex_lock.mutex();
    NOTF_ASSERT(mutex);
    if (!mutex->is_locked_by_this_thread()) { glfwMakeContextCurrent(nullptr); }
}

// stencil mask ===================================================================================================== //

void GraphicsContext::_StencilMask::operator=(StencilMask mask) {
    if (m_mask == mask) { return; }
    m_mask = mask;

    if (m_mask.front == m_mask.back) {
        NOTF_CHECK_GL(glStencilMaskSeparate(to_number(CullFace::BOTH), m_mask.front));
    } else {
        NOTF_CHECK_GL(glStencilMaskSeparate(to_number(CullFace::FRONT), m_mask.front));
        NOTF_CHECK_GL(glStencilMaskSeparate(to_number(CullFace::BACK), m_mask.back));
    }
}

// blend mode ======================================================================================================= //

void GraphicsContext::_BlendMode::operator=(const BlendMode mode) {
    if (mode == m_mode) { return; }

    if (m_mode == BlendMode::OFF) { NOTF_CHECK_GL(glEnable(GL_BLEND)); }
    m_mode = mode;

    if (m_mode == BlendMode::OFF) {
        NOTF_CHECK_GL(glDisable(GL_BLEND));
    } else {
        const BlendMode::OpenGLBlendMode blend_mode(m_mode);
        NOTF_CHECK_GL(glBlendFuncSeparate(blend_mode.source_rgb, blend_mode.destination_rgb, //
                                          blend_mode.source_alpha, blend_mode.destination_alpha));
    }
}

// face culling ===================================================================================================== //

void GraphicsContext::_CullFace::operator=(const CullFace mode) {
    if (mode == m_mode) { return; }

    if (m_mode == CullFace::NONE) { NOTF_CHECK_GL(glEnable(GL_CULL_FACE)); }
    m_mode = mode;

    if (m_mode == CullFace::NONE) {
        NOTF_CHECK_GL(glDisable(GL_CULL_FACE));
    } else {
        NOTF_CHECK_GL(glCullFace(to_number(m_mode)));
    }
}

// framebuffer binding ============================================================================================== //

void GraphicsContext::_FrameBuffer::operator=(FrameBufferPtr framebuffer) {
    if (m_framebuffer == framebuffer) { return; }
    if (framebuffer
        && FrameBuffer::AccessFor<GraphicsContext>::get_graphics_context(*framebuffer.get()) != &m_context) {
        NOTF_THROW(ValueError, "GraphicsContext of the FrameBuffer does not match.");
    }
    m_framebuffer = std::move(framebuffer);

    if (m_framebuffer) {
        NOTF_CHECK_GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer->get_id().get_value()));
    } else {
        NOTF_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
}

Size2i GraphicsContext::_FrameBuffer::get_size() const {
    Size2i result;
    glfwGetFramebufferSize(m_context.m_window, &result[0], &result[1]);
    // TODO: glfwGetFramebufferSize must only be called from the main thread
    //       - or even better: the framebuffer size is actively updated with events, pushed rather than pulled
    return result;
}

const Aabri& GraphicsContext::_FrameBuffer::get_render_area() const { return m_context.m_state._render_area; }

void GraphicsContext::_FrameBuffer::set_render_area(Aabri area, const bool force) {
    if (!area.is_valid()) { NOTF_THROW(ValueError, "Cannot set to an invalid render area"); }
    if (area != m_context.m_state._render_area || force) {
        NOTF_CHECK_GL(glViewport(area.left(), area.bottom(), area.get_width(), area.get_height()));
        m_context.m_state._render_area = std::move(area);
    }
}

void GraphicsContext::_FrameBuffer::clear(Color color, const GLBuffers buffers) {
    if (color != m_context.m_state._clear_color) {
        Color& clear_color = m_context.m_state._clear_color;
        clear_color = std::move(color);
        NOTF_CHECK_GL(glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a));
    }

    GLenum gl_flags = 0;
    if (buffers & GLBuffer::COLOR) { gl_flags |= GL_COLOR_BUFFER_BIT; }
    if (buffers & GLBuffer::DEPTH) { gl_flags |= GL_DEPTH_BUFFER_BIT; }
    if (buffers & GLBuffer::STENCIL) { gl_flags |= GL_STENCIL_BUFFER_BIT; }
    NOTF_CHECK_GL(glClear(gl_flags));
}

// shader program binding =========================================================================================== //

void GraphicsContext::_ShaderProgram::operator=(ShaderProgramPtr program) {
    if (m_program == program) { return; }
    if (program && ShaderProgram::AccessFor<GraphicsContext>::get_graphics_context(*program.get()) != &m_context) {
        NOTF_THROW(ValueError, "GraphicsContext of the ShaderProgram does not match.");
    }

    m_program = std::move(program);

    NOTF_CHECK_GL(glUseProgram(0));
    if (m_program) {
        NOTF_CHECK_GL(glBindProgramPipeline(m_program->get_id().get_value()));
    } else {
        NOTF_CHECK_GL(glBindProgramPipeline(0));
    }
}

// vertex object binding ============================================================================================ //

void GraphicsContext::_VertexObject::operator=(VertexObjectPtr vertex_object) {
    if (m_vertex_object == vertex_object) { return; }
    if (vertex_object
        && VertexObject::AccessFor<GraphicsContext>::get_graphics_context(*vertex_object.get()) != &m_context) {
        NOTF_THROW(ValueError, "GraphicsContext of the VertexObject does not match.");
    }
    m_vertex_object = std::move(vertex_object);

    if (m_vertex_object) {
        NOTF_CHECK_GL(glBindVertexArray(m_vertex_object->get_id().get_value()));
    } else {
        NOTF_CHECK_GL(glBindVertexArray(0));
    }
}

// texture slots ==================================================================================================== //

void GraphicsContext::_TextureSlots::Slot::operator=(TexturePtr texture) {
    if (m_texture == texture) { return; }
    m_texture = std::move(texture);

    NOTF_CHECK_GL(glActiveTexture(GL_TEXTURE0 + m_index));
    if (m_texture) {
        NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, m_texture->get_id().get_value()));
    } else {
        NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));
    }
}

GraphicsContext::_TextureSlots::Slot& GraphicsContext::_TextureSlots::operator[](const GLuint index) {
    static const GLuint slot_count = TheGraphicsSystem::get_environment().texture_slot_count;
    if (index >= slot_count) {
        NOTF_THROW(IndexError, "Invalid texture slot: {} - largest texture slot is: {}", index, slot_count - 1);
    }
    auto itr = m_slots.find(index);
    if (itr == m_slots.end()) {
        bool success = false;
        std::tie(itr, success) = m_slots.emplace(std::make_pair(index, index));
        NOTF_ASSERT(success);
    }
    return itr->second;
}

// uniform slots ==================================================================================================== //

void GraphicsContext::_UniformSlots::Slot::BufferBinding::_set(AnyUniformBufferPtr buffer, size_t offset) {
    if (buffer == m_buffer && offset == m_offset) { return; }
    m_buffer = std::move(buffer);
    m_offset = offset;

    if (m_buffer) {
        const GLsizeiptr block_size = narrow_cast<GLsizeiptr>(m_buffer->get_element_size());
        const GLintptr buffer_offset = block_size * narrow_cast<GLsizei>(m_offset);
        NOTF_CHECK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, m_slot_index, m_buffer->get_id().get_value(), buffer_offset,
                                        block_size));
    } else {
        NOTF_CHECK_GL(glBindBufferBase(GL_UNIFORM_BUFFER, m_slot_index, 0));
    }
}

GraphicsContext::_UniformSlots::Slot::BlockBinding::BlockBinding(ShaderProgramConstPtr program,
                                                                 const GLuint block_index, const GLuint slot_index)
    : m_program(std::move(program)), m_block_index(block_index) {
    const UniformBlock& block = m_program->get_uniform_block(m_block_index);

    if (block.get_stages() & AnyShader::Stage::VERTEX) {
        const VertexShaderPtr& vertex_shader = block.get_program().get_vertex_shader();
        NOTF_ASSERT(vertex_shader);
        m_vertex_shader_id = vertex_shader->get_id();
    }
    if (block.get_stages() & AnyShader::Stage::FRAGMENT) {
        const FragmentShaderPtr& fragment_shader = block.get_program().get_fragment_shader();
        NOTF_ASSERT(fragment_shader);
        m_fragment_shader_id = fragment_shader->get_id();
    }

    _set(slot_index);
}

void GraphicsContext::_UniformSlots::Slot::BlockBinding::_set(const GLuint slot_index) {
    if (m_vertex_shader_id) { glUniformBlockBinding(m_vertex_shader_id.get_value(), m_block_index, slot_index); }
    if (m_fragment_shader_id) { glUniformBlockBinding(m_fragment_shader_id.get_value(), m_block_index, slot_index); }
}

void GraphicsContext::_UniformSlots::Slot::bind_block(const UniformBlock& block) {
    ShaderProgramConstPtr program = block.get_program().shared_from_this();
    const GLuint block_index = block.get_index();
    for (const auto& block_binding : m_blocks) {
        if (block_binding.m_program == program && block_binding.m_block_index == block_index) { return; }
    }
    m_blocks.emplace_back(BlockBinding{std::move(program), block_index, m_buffer.m_slot_index});
}

GraphicsContext::_UniformSlots::Slot& GraphicsContext::_UniformSlots::operator[](const GLuint index) {
    static const GLuint slot_count = TheGraphicsSystem::get_environment().uniform_slot_count;
    if (index >= slot_count) {
        NOTF_THROW(IndexError, "Invalid uniform slot: {} - largest uniform slot is: {}", index, slot_count - 1);
    }
    auto itr = m_slots.find(index);
    if (itr == m_slots.end()) {
        bool success = false;
        std::tie(itr, success) = m_slots.emplace(std::make_pair(index, index));
        NOTF_ASSERT(success);
    }
    return itr->second;
}

// graphics context ================================================================================================= //

GraphicsContext::GraphicsContext(std::string name, valid_ptr<GLFWwindow*> window)
    : m_name(std::move(name)), m_window(window), m_state(*this) {
    NOTF_GUARD(make_current());

    { // sanity check (otherwise OpenGL may happily return `null` on all calls until something crashes later)
        const GLubyte* version;
        NOTF_CHECK_GL(version = glGetString(GL_VERSION));
        if (!version) { NOTF_THROW(OpenGLError, "Failed to create OpenGL context \"{}\"", m_name); }
    }

    // GLFW hints
    glfwSwapInterval(0);

    // OpenGL hints
    NOTF_CHECK_GL(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
    NOTF_CHECK_GL(glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_DONT_CARE));
    NOTF_CHECK_GL(glDisable(GL_DITHER));
}

GraphicsContext::~GraphicsContext() {
    NOTF_GUARD(make_current());

    // shed all owning pointers from the current state
    reset();

    // deallocate and invalidate all remaining ShaderPrograms
    for (auto itr : m_programs) {
        if (ShaderProgramPtr program = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live ShaderProgram \"{}\" from GraphicsContext \"{}\"", program->get_name(),
                          m_name);
            ShaderProgram::AccessFor<GraphicsContext>::deallocate(*program);
        }
    }
    m_programs.clear();

    // deallocate and invalidate all remaining FrameBuffers
    for (auto itr : m_framebuffers) {
        if (FrameBufferPtr framebuffer = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live FrameBuffer \"{}\" from GraphicsContext \"{}\"", framebuffer->get_name(),
                          m_name);
            FrameBuffer::AccessFor<GraphicsContext>::deallocate(*framebuffer);
        }
    }
    m_framebuffers.clear();

    // deallocate and invalidate all remaining VertexObjects
    for (auto itr : m_vertex_objects) {
        if (VertexObjectPtr vertex_object = itr.second.lock()) {
            NOTF_LOG_WARN("Deallocating live VertexObject \"{}\" from GraphicsContext \"{}\"",
                          vertex_object->get_name(), m_name);
            VertexObject::AccessFor<GraphicsContext>::deallocate(*vertex_object);
        }
    }
    m_vertex_objects.clear();
}

GraphicsContext::Guard GraphicsContext::make_current(bool assume_is_current) {
    if (GLFWwindow* glfw_window = glfwGetCurrentContext(); glfw_window && glfw_window != m_window) {
        NOTF_THROW(ThreadError,
                   "Cannot make GraphicsContext \"{}\" current on this thread while another one is already current",
                   m_name);
    }

    // lock the context's mutex
    auto lock = std::unique_lock<RecursiveMutex>(m_mutex, std::defer_lock_t());
    if (assume_is_current) {
        if (!lock.try_lock()) {
            NOTF_LOG_WARN("Assumption failed: GraphicsContext \"{}\" was not current when expected", m_name);
            lock.lock();
        }
    } else {
        lock.lock();
    }
    NOTF_ASSERT(lock.owns_lock());

    // make the context current if this is the first lock on this thread
    if (m_mutex.get_recursion_counter() == 1) { glfwMakeContextCurrent(m_window); }
    NOTF_ASSERT(glfwGetCurrentContext() == m_window);

    return Guard(*this, std::move(lock));
}

void GraphicsContext::begin_frame() {
    const auto current_start_time = get_now();
    if (const auto difference
        = std::chrono::duration_cast<std::chrono::nanoseconds>(current_start_time - m_frame_start_time);
        difference > 16666666ns) {
        NOTF_LOG_WARN("Frame start difference of {}ms detected", difference.count() / 1000000.);
    }
    m_frame_start_time = current_start_time;

    m_state.framebuffer.clear(m_state._clear_color, GLBuffer::COLOR | GLBuffer::DEPTH | GLBuffer::STENCIL);
}

void GraphicsContext::finish_frame() {
    NOTF_ASSERT(is_current());
    glfwSwapBuffers(m_window);

    if (const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(get_now() - m_frame_start_time);
        duration > 16666666ns) {
        NOTF_LOG_WARN("Frame took {}ms to draw", duration.count() / 1000000.);
    }
}

void GraphicsContext::reset() {
    m_state.blend_mode = BlendMode();
    m_state.cull_face = CullFace::DEFAULT;
    m_state.stencil_mask = StencilMask();
    m_state.program = nullptr;
    m_state.vertex_object = nullptr;
    m_state.framebuffer = nullptr;
    m_state.texture_slots.clear();
    m_state.uniform_slots.clear();
    m_state._clear_color = Color::black();
    m_state.framebuffer.set_render_area(m_state.framebuffer.get_size());
}

void GraphicsContext::_register_new(ShaderProgramPtr program) {
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

void GraphicsContext::_register_new(FrameBufferPtr framebuffer) {
    auto it = m_framebuffers.find(framebuffer->get_id());
    if (it == m_framebuffers.end()) {
        m_framebuffers.emplace(framebuffer->get_id(), framebuffer); // insert new
    } else if (it->second.expired()) {
        it->second = framebuffer; // update expired
    } else {
        NOTF_THROW(
            NotUniqueError,
            "GraphicsContext failed to register a new FrameBuffer with the same ID as an existing FrameBuffer: \"{}\"",
            framebuffer->get_id());
    }
}

void GraphicsContext::_register_new(VertexObjectPtr vertex_object) {
    auto it = m_vertex_objects.find(vertex_object->get_id());
    if (it == m_vertex_objects.end()) {
        m_vertex_objects.emplace(vertex_object->get_id(), vertex_object); // insert new
    } else if (it->second.expired()) {
        it->second = vertex_object; // update expired
    } else {
        NOTF_THROW(
            NotUniqueError,
            "GraphicsContext failed to register a new VertexObject with the same ID as an existing VertexObject: \"{}\"",
            vertex_object->get_id());
    }
}

/* Something to think of, courtesy of the OpenGL ES book:
 * What happens if we are rendering into a texture and at the same time use this texture object as a texture in a
 * fragment shader? Will the OpenGL ES implementation generate an error when such a situation arises? In some cases,
 * it is possible for the OpenGL ES implementation to determine if a texture object is being used as a texture input
 * and a framebuffer attachment into which we are currently drawing. glDrawArrays and glDrawElements could then
 * generate an error. To ensure that glDrawArrays and glDrawElements can be executed as rapidly as possible,
 * however, these checks are not performed. Instead of generating an error, in this case rendering results are
 * undefined. It is the application’s responsibility to make sure that this situation does not occur.
 */
