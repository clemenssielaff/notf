#include "notf/graphic/renderer/fragment_renderer.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/shader_program.hpp"

NOTF_OPEN_NAMESPACE

FragmentRenderer::FragmentRenderer(valid_ptr<VertexShaderPtr> vertex_shader,
                                   valid_ptr<FragmentShaderPtr> fragment_shader)
    : m_program(ShaderProgram::create("FragmentRenderer", std::move(vertex_shader), std::move(fragment_shader)))
    , m_fragment_shader(m_program->get_fragment_shader()) {}

void FragmentRenderer::render() const {
    GraphicsContext& context = GraphicsContext::get();
    const auto program_guard = context.bind_program(m_program);
    NOTF_CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 3));
}

NOTF_CLOSE_NAMESPACE
