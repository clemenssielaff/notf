#include "notf/graphic/renderer/fragment_renderer.hpp"

#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/shader_program.hpp"

NOTF_OPEN_NAMESPACE

FragmentRenderer::FragmentRenderer(GraphicsContext& context, valid_ptr<VertexShaderPtr> vertex_shader,
                                   valid_ptr<FragmentShaderPtr> fragment_shader)
    : m_program(
        ShaderProgram::create(context, "FragmentRenderer", std::move(vertex_shader), std::move(fragment_shader))) {}

void FragmentRenderer::render() const {
    m_program->get_context()->program = m_program;
    NOTF_CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 3));
}

NOTF_CLOSE_NAMESPACE
