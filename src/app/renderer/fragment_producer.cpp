#include "app/renderer/fragment_producer.hpp"

#include "common/system.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"

NOTF_OPEN_NAMESPACE

FragmentProducer::FragmentProducer(const Token& token, SceneManagerPtr& manager, const std::string& shader)
    : GraphicsProducer(token), m_pipeline(), m_context(*manager->graphics_context())
{
    GraphicsContextPtr& context = manager->graphics_context();

    // TODO: we need a resource manager! This "loading from file" all the time won't fly!
    const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/fullscreen.vert");
    VertexShaderPtr vertex_shader = VertexShader::create(context, "fullscreen.vert", vertex_src);

    const std::string frag_src    = load_file(shader);
    FragmentShaderPtr frag_shader = FragmentShader::create(context, "fragment_producer.frag", frag_src);

    m_pipeline = Pipeline::create(context, vertex_shader, frag_shader);
}

void FragmentProducer::_render() const
{
    const auto pipeline_guard = m_context.bind_pipeline(m_pipeline);
    gl_check(glDrawArrays(GL_TRIANGLES, 0, 3));
}

NOTF_CLOSE_NAMESPACE
