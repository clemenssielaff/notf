#include "notf/graphic/shader_program.hpp"

#include <sstream>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/shader.hpp"

NOTF_OPEN_NAMESPACE

ShaderProgram::ShaderProgram(VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                             GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader)
    : m_vertex_shader(std::move(vertex_shader))
    , m_tesselation_shader(std::move(tesselation_shader))
    , m_geometry_shader(std::move(geometry_shader))
    , m_fragment_shader(std::move(fragment_shader)) {
    { // generate new shader program id
        NOTF_CHECK_GL(glGenProgramPipelines(1, &m_id.data()));
        if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new ShaderProgram"); }
    }

    std::vector<std::string> attached_stages;
    attached_stages.reserve(4);

    if (m_vertex_shader) {
        NOTF_ASSERT(m_vertex_shader->is_valid());
        NOTF_CHECK_GL(glUseProgramStages(m_id.value(), GL_VERTEX_SHADER_BIT, m_vertex_shader->get_id().value()));

        std::stringstream ss;
        ss << "vertex shader \"" << m_vertex_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_tesselation_shader) {
        NOTF_ASSERT(m_tesselation_shader->is_valid());
        NOTF_CHECK_GL(glUseProgramStages(m_id.value(), GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT,
                                         m_tesselation_shader->get_id().value()));

        std::stringstream ss;
        ss << "tesselation shader \"" << m_tesselation_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_geometry_shader) {
        NOTF_ASSERT(m_geometry_shader->is_valid());
        NOTF_CHECK_GL(glUseProgramStages(m_id.value(), GL_GEOMETRY_SHADER_BIT, m_geometry_shader->get_id().value()));

        std::stringstream ss;
        ss << "geometry shader \"" << m_geometry_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_fragment_shader) {
        NOTF_ASSERT(m_fragment_shader->is_valid());
        NOTF_CHECK_GL(glUseProgramStages(m_id.value(), GL_FRAGMENT_SHADER_BIT, m_fragment_shader->get_id().value()));

        std::stringstream ss;
        ss << "fragment shader \"" << m_fragment_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    { // validate the program once it has been created
        NOTF_CHECK_GL(glValidateProgramPipeline(m_id.value()));
        GLint is_valid = 0;
        NOTF_CHECK_GL(glGetProgramPipelineiv(m_id.value(), GL_VALIDATE_STATUS, &is_valid));
        if (!is_valid) {
            GLint log_length = 0;
            glGetProgramPipelineiv(m_id.value(), GL_INFO_LOG_LENGTH, &log_length);
            std::string error_message;
            if (!log_length) {
                error_message = "Failed to validate the ShaderProgram";
            } else {
                error_message.resize(static_cast<size_t>(log_length), '\0');
                glGetProgramPipelineInfoLog(m_id.value(), log_length, nullptr, &error_message[0]);
                if (error_message.compare(0, 7, "error:\t") != 0) { // prettify the message for logging
                    error_message = error_message.substr(7, error_message.size() - 9);
                }
            }
            NOTF_THROW(OpenGLError, "{}", error_message);
        }
    }

    { // log information about the successful creation
        NOTF_ASSERT(attached_stages.size() > 0 && attached_stages.size() < 5);
        std::stringstream message;
        message << "Created new Pipline: " << m_id << " with ";
        if (attached_stages.size() == 1) {
            message << attached_stages[0];
        } else if (attached_stages.size() == 2) {
            message << attached_stages[0] << " and " << attached_stages[1];
        } else if (attached_stages.size() == 3) {
            message << attached_stages[0] << ", " << attached_stages[1] << " and " << attached_stages[2];
        } else if (attached_stages.size() == 4) {
            message << attached_stages[0] << ", " << attached_stages[1] << ", " << attached_stages[2] << " and "
                    << attached_stages[3];
        }
        NOTF_LOG_INFO("{}", message.str());
    }
}

ShaderProgramPtr ShaderProgram::create(VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                                       GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader) {
    ShaderProgramPtr program = _create_shared(std::move(vertex_shader), std::move(tesselation_shader),
                                              std::move(geometry_shader), std::move(fragment_shader));
    TheGraphicsSystem::AccessFor<ShaderProgram>::register_new(program);
    return program;
}

ShaderProgram::~ShaderProgram() { _deallocate(); }

void ShaderProgram::_deallocate() {
    if (m_id) {
        NOTF_CHECK_GL(glDeleteProgramPipelines(1, &m_id.value()));
        NOTF_LOG_TRACE("Deleted ShaderProgram: {}", m_id);
        m_id = ShaderProgramId::invalid();
    }
}

NOTF_CLOSE_NAMESPACE
