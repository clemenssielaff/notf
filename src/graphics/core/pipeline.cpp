#include "graphics/core/pipeline.hpp"

#include <cassert>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/shader.hpp"
#include "utils/make_smart_enabler.hpp"

NOTF_OPEN_NAMESPACE

Pipeline::Pipeline(GraphicsContext& context, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                   GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader)
    : m_graphics_context(context)
    , m_id(0)
    , m_vertex_shader(std::move(vertex_shader))
    , m_tesselation_shader(std::move(tesselation_shader))
    , m_geometry_shader(std::move(geometry_shader))
    , m_fragment_shader(std::move(fragment_shader))
{
    { // generate new pipeline id
        PipelineId::underlying_t id = 0;
        notf_check_gl(glGenProgramPipelines(1, &id));
        m_id = id;
        if (m_id == 0) {
            notf_throw(runtime_error, "Could not allocate new Pipeline");
        }
    }

    std::vector<std::string> attached_stages;
    attached_stages.reserve(4);

    if (m_vertex_shader) {
        assert(m_vertex_shader->is_valid());
        assert(m_vertex_shader->context() == m_graphics_context);
        notf_check_gl(glUseProgramStages(m_id.value(), GL_VERTEX_SHADER_BIT, m_vertex_shader->id().value()));

        std::stringstream ss;
        ss << "vertex shader \"" << m_vertex_shader->name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_tesselation_shader) {
        assert(m_tesselation_shader->is_valid());
        assert(m_tesselation_shader->context() == m_graphics_context);
        notf_check_gl(glUseProgramStages(m_id.value(), GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT,
                                    m_tesselation_shader->id().value()));

        std::stringstream ss;
        ss << "tesselation shader \"" << m_tesselation_shader->name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_geometry_shader) {
        assert(m_geometry_shader->is_valid());
        assert(m_geometry_shader->context() == m_graphics_context);
        notf_check_gl(glUseProgramStages(m_id.value(), GL_GEOMETRY_SHADER_BIT, m_geometry_shader->id().value()));

        std::stringstream ss;
        ss << "geometry shader \"" << m_geometry_shader->name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_fragment_shader) {
        assert(m_fragment_shader->is_valid());
        assert(m_fragment_shader->context() == m_graphics_context);
        notf_check_gl(glUseProgramStages(m_id.value(), GL_FRAGMENT_SHADER_BIT, m_fragment_shader->id().value()));

        std::stringstream ss;
        ss << "fragment shader \"" << m_fragment_shader->name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    { // validate the pipeline once it has been created
        notf_check_gl(glValidateProgramPipeline(m_id.value()));
        GLint is_valid = 0;
        notf_check_gl(glGetProgramPipelineiv(m_id.value(), GL_VALIDATE_STATUS, &is_valid));
        if (!is_valid) {
            GLint log_length = 0;
            glGetProgramPipelineiv(m_id.value(), GL_INFO_LOG_LENGTH, &log_length);
            std::string error_message;
            if (!log_length) {
                error_message = "Failed to validate the Pipeline";
            }
            else {
                error_message.resize(static_cast<size_t>(log_length), '\0');
                glGetProgramPipelineInfoLog(m_id.value(), log_length, nullptr, &error_message[0]);
                if (error_message.compare(0, 7, "error:\t") != 0) { // prettify the message for logging
                    error_message = error_message.substr(7, error_message.size() - 9);
                }
            }
            notf_throw(runtime_error, error_message);
        }
    }

    { // log information about the successful creation
        assert(attached_stages.size() > 0 && attached_stages.size() < 5);
        std::stringstream message;
        message << "Created new Pipline: " << m_id << " with ";
        if (attached_stages.size() == 1) {
            message << attached_stages[0];
        }
        else if (attached_stages.size() == 2) {
            message << attached_stages[0] << " and " << attached_stages[1];
        }
        else if (attached_stages.size() == 3) {
            message << attached_stages[0] << ", " << attached_stages[1] << " and " << attached_stages[2];
        }
        else if (attached_stages.size() == 4) {
            message << attached_stages[0] << ", " << attached_stages[1] << ", " << attached_stages[2] << " and "
                    << attached_stages[3];
        }
        log_info << message.str();
    }
}

PipelinePtr
Pipeline::create(GraphicsContextPtr& context, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                 GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader)
{
#ifdef NOTF_DEBUG
    return PipelinePtr(new Pipeline(*context, std::move(vertex_shader), std::move(tesselation_shader),
                                    std::move(geometry_shader), std::move(fragment_shader)));
#else
    return std::make_shared<make_shared_enabler<Pipeline>>(*context, std::move(vertex_shader),
                                                           std::move(tesselation_shader), std::move(geometry_shader),
                                                           std::move(fragment_shader));
#endif
}

Pipeline::~Pipeline()
{
    if (m_id) {
        notf_check_gl(glDeleteProgramPipelines(1, &m_id.value()));
        log_trace << "Deleted Pipeline: " << m_id;
        m_id = PipelineId::invalid();
    }
}

NOTF_CLOSE_NAMESPACE
