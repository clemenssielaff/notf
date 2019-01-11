#include "notf/graphic/shader_program.hpp"

#include <sstream>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/gl_utils.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/shader.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

#ifdef NOTF_DEBUG
void assert_is_valid(const ShaderProgram& shader) {
    if (!shader.is_valid()) {
        NOTF_THROW(ResourceError, "ShaderProgram \"{}\" was deallocated! Has TheGraphicsSystem been deleted?",
                   shader.get_name());
    }
}
#else
void assert_is_valid(const Shader&) {} // noop
#endif

/// Get the size of the longest attribute name in a program.
size_t longest_attribute_length(const GLuint program) {
    GLint result = 0;
    NOTF_CHECK_GL(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &result));
    return narrow_cast<size_t>(result);
}

} // namespace

NOTF_OPEN_NAMESPACE

ShaderProgram::ShaderProgram(std::string name, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                             GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader)
    : m_name(std::move(name))
    , m_vertex_shader(std::move(vertex_shader))
    , m_tesselation_shader(std::move(tesselation_shader))
    , m_geometry_shader(std::move(geometry_shader))
    , m_fragment_shader(std::move(fragment_shader)) {

    { // generate new shader program id
        NOTF_CHECK_GL(glGenProgramPipelines(1, &m_id.data()));
        if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new ShaderProgram \"{}\"", m_name); }
    }

    // define the program stages
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
        message << "Created new ShaderProgram: \"" << m_name << "\" with ";
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

    // collect uniforms
    for (const auto& shader :
         std::array<ShaderPtr, 4>{m_vertex_shader, m_tesselation_shader, m_geometry_shader, m_fragment_shader}) {
        if (!shader) { continue; }
        for (const auto& uniform : shader->get_uniforms()) {
            m_uniforms.emplace_back(Uniform{shader->get_stage(), uniform});
        }
    }

    { // discover attributes
        GLint attribute_count = 0;
        NOTF_CHECK_GL(glGetProgramiv(get_id().value(), GL_ACTIVE_ATTRIBUTES, &attribute_count));
        NOTF_ASSERT(attribute_count >= 0);
        m_attributes.reserve(static_cast<size_t>(attribute_count));

        std::vector<GLchar> uniform_name(longest_attribute_length(get_id().value()));
        for (GLuint index = 0; index < static_cast<GLuint>(attribute_count); ++index) {
            Attribute attribute;
            attribute.type = 0;
            attribute.size = 0;

            GLsizei name_length = 0;
            NOTF_CHECK_GL(glGetActiveAttrib(get_id().value(), index, static_cast<GLsizei>(uniform_name.size()),
                                            &name_length, &attribute.size, &attribute.type, &uniform_name[0]));
            NOTF_ASSERT(attribute.type);
            NOTF_ASSERT(attribute.size);

            NOTF_ASSERT(name_length > 0);
            attribute.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

            // some variable names are pre-defined by the language and cannot be set by the user
            if (attribute.name == "gl_VertexID") { continue; }

            attribute.location = glGetAttribLocation(get_id().value(), attribute.name.c_str());
            NOTF_ASSERT(attribute.location >= 0);

            NOTF_LOG_TRACE("Discovered attribute \"{}\" on shader: \"{}\"", attribute.name, get_name());
            m_attributes.emplace_back(std::move(attribute));
        }
        m_attributes.shrink_to_fit();
    }
}

ShaderProgramPtr
ShaderProgram::create(std::string name, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                      GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader) {
    ShaderProgramPtr program = _create_shared(std::move(name), std::move(vertex_shader), std::move(tesselation_shader),
                                              std::move(geometry_shader), std::move(fragment_shader));
    TheGraphicsSystem::AccessFor<ShaderProgram>::register_new(program);
    return program;
}

ShaderProgram::~ShaderProgram() { _deallocate(); }

bool ShaderProgram::is_valid() const {
    if (m_vertex_shader && !m_vertex_shader->is_valid()) { return false; }
    if (m_tesselation_shader && !m_tesselation_shader->is_valid()) { return false; }
    if (m_geometry_shader && !m_geometry_shader->is_valid()) { return false; }
    if (m_fragment_shader && !m_fragment_shader->is_valid()) { return false; }
    return true;
}

ShaderProgram::Uniform& ShaderProgram::_get_uniform(const std::string& name) {
    assert_is_valid(*this);

    auto it = std::find_if(std::begin(m_uniforms), std::end(m_uniforms),
                           [&](const auto& uniform) -> bool { return uniform.variable.name == name; });
    if (it == std::end(m_uniforms)) {
        NOTF_THROW(OpenGLError, "No uniform named \"{}\" in shader \"{}\"", name, m_name);
    }
    return *it;
}

void ShaderProgram::_deallocate() {
    if (m_id) {
        NOTF_CHECK_GL(glDeleteProgramPipelines(1, &m_id.value()));
        NOTF_LOG_TRACE("Deleted ShaderProgram: {}", m_name);
        m_id = ShaderProgramId::invalid();
    }
}

NOTF_CLOSE_NAMESPACE
