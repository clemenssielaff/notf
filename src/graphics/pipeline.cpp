#include "graphics/pipeline.hpp"

#include <assert.h>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/shader.hpp"

namespace notf {

Pipeline::Pipeline(GraphicsContext& context, VertexShaderPtr vertex_shader, GeometryShaderPtr geometry_shader,
                   FragmentShaderPtr fragment_shader)
    : m_graphics_context(context)
    , m_id(0)
    , m_vertex_shader(std::move(vertex_shader))
    , m_geometry_shader(std::move(geometry_shader))
    , m_fragment_shader(std::move(fragment_shader))
{
    gl_check(glGenProgramPipelines(1, &m_id));
    assert(m_id);

    std::stringstream message;
    message << "Created new Pipline: " << m_id << ",";

    glBindProgramPipeline(m_id);

    if (m_vertex_shader) {
        assert(m_vertex_shader->is_valid());
        assert(m_vertex_shader->context() == m_graphics_context);
        gl_check(glUseProgramStages(m_id, GL_VERTEX_SHADER_BIT, m_vertex_shader->id()));

        message << " with vertex shader \"" << m_vertex_shader->name() << "\"";
    }

    if (m_geometry_shader) {
        assert(m_geometry_shader->is_valid());
        assert(m_geometry_shader->context() == m_graphics_context);
        gl_check(glUseProgramStages(m_id, GL_GEOMETRY_SHADER_BIT, m_geometry_shader->id()));

        message << (m_vertex_shader ? (m_fragment_shader ? ", " : " and ") : " with ");
        message << "geometry shader \"" << m_geometry_shader->name() << "\"";
    }

    if (m_fragment_shader) {
        assert(m_fragment_shader->is_valid());
        assert(m_fragment_shader->context() == m_graphics_context);
        gl_check(glUseProgramStages(m_id, GL_FRAGMENT_SHADER_BIT, m_fragment_shader->id()));

        message << ((m_vertex_shader || m_geometry_shader) ? " and " : " with ");
        message << "fragment shader \"" << m_fragment_shader->name() << "\"";
    }

    { // validate the pipeline once it has been created
        gl_check(glValidateProgramPipeline(m_id));
        GLint is_valid = 0;
        gl_check(glGetProgramPipelineiv(m_id, GL_VALIDATE_STATUS, &is_valid));
        if (is_valid) {
            std::stringstream ss;
            ss << "Failed to validate pipeline";
            throw_runtime_error(ss.str());
        }
    }

    log_info << message.str();
}

PipelinePtr Pipeline::create(GraphicsContext& context, VertexShaderPtr vertex_shader, GeometryShaderPtr geometry_shader,
                             FragmentShaderPtr fragment_shader)
{
#ifdef NOTF_DEBUG
    return PipelinePtr(
        new Pipeline(context, std::move(vertex_shader), std::move(geometry_shader), std::move(fragment_shader)));
#else
    struct make_shared_enabler : public Pipeline {
        make_shared_enabler(GraphicsContext& context, VertexShaderPtr vertex_shader, GeometryShaderPtr geometry_shader,
                            FragmentShaderPtr fragment_shader)
            : Pipeline(context, std::move(vertex_shader), std::move(geometry_shader), std::move(fragment_shader))
        {}
    };
    return std::make_shared<make_shared_enabler>(context, std::move(vertex_shader), std::move(geometry_shader),
                                                 std::move(fragment_shader));
#endif
}

Pipeline::~Pipeline()
{
    if (m_id) {
        gl_check(glDeleteProgramPipelines(1, &m_id));
        log_trace << "Deleted Pipeline: " << m_id;
    }
    m_id = 0;
}

} // namespace notf
