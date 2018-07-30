#include "graphics/renderer/fragment_renderer.hpp"

#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/pipeline.hpp"

NOTF_OPEN_NAMESPACE

FragmentRenderer::FragmentRenderer(valid_ptr<VertexShaderPtr> vertex_shader,
                                   valid_ptr<FragmentShaderPtr> fragment_shader)
    : m_context(vertex_shader->get_context())
    , m_pipeline(Pipeline::create(m_context, std::move(vertex_shader), std::move(fragment_shader)))
    , m_fragment_shader(m_pipeline->get_fragment_shader())
{}

void FragmentRenderer::render() const
{
    const auto pipeline_guard = m_context.bind_pipeline(m_pipeline);
    notf_check_gl(glDrawArrays(GL_TRIANGLES, 0, 3));
}

NOTF_CLOSE_NAMESPACE
