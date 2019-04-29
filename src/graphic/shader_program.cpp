#include "notf/graphic/shader_program.hpp"

#include <sstream>

#include "notf/meta/log.hpp"

#include "notf/common/geo/matrix4.hpp"
#include "notf/common/geo/vector4.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/graphics_context.hpp"

NOTF_USING_NAMESPACE;

namespace { // anonymous

void assert_is_valid(const ShaderProgram& shader) {
    if constexpr (config::is_debug_build()) {
        if (!shader.is_valid()) {
            NOTF_THROW(ResourceError, "ShaderProgram \"{}\" was deallocated! Has its GraphicsContext been deleted?",
                       shader.get_name());
        }
    } else {
        // noop
    }
}

AnyShader::Stage::Flags get_uniform_block_stages(const GLuint program, const GLuint block_index) {
    GLint is_in_vertex_shader = 0;
    NOTF_CHECK_GL(glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER,
                                            &is_in_vertex_shader));

    GLint is_in_fragment_shader = 0;
    NOTF_CHECK_GL(glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER,
                                            &is_in_fragment_shader));

    AnyShader::Stage::Flags stages = 0;
    if (is_in_vertex_shader) { stages &= AnyShader::Stage::VERTEX; }
    if (is_in_fragment_shader) { stages &= AnyShader::Stage::FRAGMENT; }
    return stages;
}

GLuint get_uniform_block_size(const GLuint program, const GLuint block_index) {
    GLint data_size = 0;
    NOTF_CHECK_GL(glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &data_size));
    NOTF_ASSERT(data_size >= 0);
    return static_cast<GLuint>(data_size);
}

std::string get_uniform_block_name(const GLuint program, const GLuint block_index) {
    GLint name_length = 0;
    NOTF_CHECK_GL(glGetActiveUniformBlockiv(program, block_index, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_length));
    std::string result;
    if (name_length > 1) {
        result.resize(static_cast<GLuint>(name_length) - 1); // ignore trailing zero
        GLsizei ignore;
        NOTF_CHECK_GL(glGetActiveUniformBlockName(program, block_index, name_length, &ignore, &result.front()));
    }
    return result;
}

} // namespace

// uniform block ==================================================================================================== //

namespace notf::detail {

UniformBlock::UniformBlock(const ShaderProgram& program, const AnyShaderPtr& shader, const GLuint index)
    : m_program(program)
    , m_name(get_uniform_block_name(shader->get_id().get_value(), index))
    , m_index(index)
    , m_stages(get_uniform_block_stages(shader->get_id().get_value(), index))
    , m_data_size(get_uniform_block_size(shader->get_id().get_value(), index)) {

    { // variables
        GLuint shader_id = shader->get_id().get_value();
        GLuint variable_count = 0;
        NOTF_CHECK_GL(glGetActiveUniformBlockiv(shader_id, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
                                                reinterpret_cast<GLint*>(&variable_count)));
        if (variable_count > 0) {
            std::vector<GLuint> indices(variable_count);
            NOTF_CHECK_GL(glGetActiveUniformBlockiv(shader_id, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                                    reinterpret_cast<GLint*>(&indices.front())));
            m_variables.resize(variable_count);
            for (size_t i = 0; i < variable_count; ++i) {
                m_variables[i].index = indices[i];
            }
        }
    }
}

} // namespace notf::detail

// shader program =================================================================================================== //

ShaderProgram::ShaderProgram(GraphicsContext& context, std::string name, VertexShaderPtr vert_shader,
                             TesselationShaderPtr tess_shader, GeometryShaderPtr geo_shader,
                             FragmentShaderPtr frag_shader)
    : m_context(context)
    , m_name(std::move(name))
    , m_vertex_shader(std::move(vert_shader))
    , m_tesselation_shader(std::move(tess_shader))
    , m_geometry_shader(std::move(geo_shader))
    , m_fragment_shader(std::move(frag_shader)) {

    NOTF_GUARD(m_context.make_current());

    _link_program();

    // discover uniform blocks
    for (const auto& shader : auto_list<AnyShaderPtr>(m_vertex_shader, m_fragment_shader)) {
        if (shader) { _find_uniform_blocks(shader); }
    }

    // discover uniforms
    for (const auto& shader :
         auto_list<AnyShaderPtr>(m_vertex_shader, m_tesselation_shader, m_geometry_shader, m_fragment_shader)) {
        if (shader) { _find_uniforms(shader); }
    }

    _find_attributes();
}

ShaderProgramPtr ShaderProgram::create(GraphicsContext& context, std::string name, VertexShaderPtr vert_shader,
                                       TesselationShaderPtr tesselation_shader, GeometryShaderPtr geometry_shader,
                                       FragmentShaderPtr frag_shader) {
    ShaderProgramPtr program = _create_shared(context, name, std::move(vert_shader), std::move(tesselation_shader),
                                              std::move(geometry_shader), std::move(frag_shader));
    GraphicsContext::AccessFor<ShaderProgram>::register_new(context, program);
    ResourceManager::get_instance().get_type<ShaderProgram>().set(std::move(name), program);
    return program;
}

ShaderProgram::~ShaderProgram() { _deallocate(); }

const ShaderProgram::Uniform& ShaderProgram::get_uniform(const std::string& name) const {
    assert_is_valid(*this);
    for (auto& uniform : m_uniforms) {
        if (name == uniform.get_name()) { return uniform; }
    }
    NOTF_THROW(NameError, "No Uniform named \"{}\" in ShaderProgram \"{}\"", name, m_name);
}

const ShaderProgram::Uniform& ShaderProgram::get_uniform(const GLint location) const {
    assert_is_valid(*this);
    for (auto& uniform : m_uniforms) {
        if (location == uniform.get_location()) { return uniform; }
    }
    NOTF_THROW(IndexError, "No Uniform at location \"{}\" in ShaderProgram \"{}\"", location, m_name);
}

const ShaderProgram::UniformBlock& ShaderProgram::get_uniform_block(const std::string& name) const {
    assert_is_valid(*this);
    for (auto& block : m_uniform_blocks) {
        if (name == block.get_name()) { return block; }
    }
    NOTF_THROW(NameError, "No UniformBlock named \"{}\" in ShaderProgram \"{}\"", name, m_name);
}

const ShaderProgram::UniformBlock& ShaderProgram::get_uniform_block(const GLuint index) const {
    assert_is_valid(*this);
    for (auto& block : m_uniform_blocks) {
        if (index == block.get_index()) { return block; }
    }
    NOTF_THROW(IndexError, "No UniformBlock with index \"{}\" in ShaderProgram \"{}\"", index, m_name);
}

void ShaderProgram::_link_program() {
    // generate new shader program id
    NOTF_CHECK_GL(glGenProgramPipelines(1, &m_id.data()));
    if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new ShaderProgram \"{}\"", m_name); }

    // define the program stages
    std::vector<std::string> attached_stages;
    attached_stages.reserve(4);

    if (m_vertex_shader) {
        NOTF_ASSERT(m_vertex_shader->is_valid());
        NOTF_CHECK_GL(
            glUseProgramStages(m_id.get_value(), GL_VERTEX_SHADER_BIT, m_vertex_shader->get_id().get_value()));

        std::stringstream ss;
        ss << "vertex shader \"" << m_vertex_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_tesselation_shader) {
        NOTF_ASSERT(m_tesselation_shader->is_valid());
        NOTF_CHECK_GL(glUseProgramStages(m_id.get_value(), GL_TESS_CONTROL_SHADER_BIT | GL_TESS_EVALUATION_SHADER_BIT,
                                         m_tesselation_shader->get_id().get_value()));

        std::stringstream ss;
        ss << "tesselation shader \"" << m_tesselation_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_geometry_shader) {
        NOTF_ASSERT(m_geometry_shader->is_valid());
        NOTF_CHECK_GL(
            glUseProgramStages(m_id.get_value(), GL_GEOMETRY_SHADER_BIT, m_geometry_shader->get_id().get_value()));

        std::stringstream ss;
        ss << "geometry shader \"" << m_geometry_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    if (m_fragment_shader) {
        NOTF_ASSERT(m_fragment_shader->is_valid());
        NOTF_CHECK_GL(
            glUseProgramStages(m_id.get_value(), GL_FRAGMENT_SHADER_BIT, m_fragment_shader->get_id().get_value()));

        std::stringstream ss;
        ss << "fragment shader \"" << m_fragment_shader->get_name() << "\"";
        attached_stages.emplace_back(ss.str());
    }

    { // validate the program once it has been created
        NOTF_CHECK_GL(glValidateProgramPipeline(m_id.get_value()));
        GLint is_valid = 0;
        NOTF_CHECK_GL(glGetProgramPipelineiv(m_id.get_value(), GL_VALIDATE_STATUS, &is_valid));
        if (!is_valid) {
            GLint log_length = 0;
            NOTF_CHECK_GL(glGetProgramPipelineiv(m_id.get_value(), GL_INFO_LOG_LENGTH, &log_length));
            std::string error_message;
            if (log_length) {
                // if the driver is helpful enough to supply an error message, use that
                error_message.resize(static_cast<size_t>(log_length), '\0');
                NOTF_CHECK_GL(glGetProgramPipelineInfoLog(m_id.get_value(), log_length, nullptr, &error_message[0]));
                if (error_message.compare(0, 7, "error:\t") != 0) { // prettify the message for logging
                    error_message = error_message.substr(7, error_message.size() - 9);
                }
            } else {
                // Unfortunately, it seems that the OpenGL implementations that I use do not supply any additional
                // information on why pipeline validation would fail. But even they do tell you why linking a Shader
                // failed. So what we do here is to link a single Shader with all the same sources as the pipeline and
                // see if that fails with a more helpful error message.

                // collect all information required to re-build the pipeline as a MultiStageShader
                MultiStageShader::Sources stages;
                std::optional<AnyShader::Definitions> definitions;
                bool warned_once = false;
                if (m_vertex_shader) {
                    definitions = m_vertex_shader->get_definitions();
                    stages.vertex = m_vertex_shader->get_source();
                }
                if (m_tesselation_shader) {
                    if (definitions.has_value()) {
                        if (definitions.value() != m_tesselation_shader->get_definitions()) {
                            NOTF_LOG_WARN("Shader Program \"{}\" uses shaders with different definitions", m_name);
                            warned_once = true;
                        }
                    } else {
                        definitions = m_tesselation_shader->get_definitions();
                    }
                    stages.tesselation_control = m_tesselation_shader->get_control_source();
                    stages.tesselation_evaluation = m_tesselation_shader->get_evaluation_source();
                }
                if (m_geometry_shader) {
                    if (definitions.has_value()) {
                        if (definitions.value() != m_geometry_shader->get_definitions() && !warned_once) {
                            NOTF_LOG_WARN("Shader Program \"{}\" uses shaders with different definitions", m_name);
                            warned_once = true;
                        }
                    } else {
                        definitions = m_geometry_shader->get_definitions();
                    }
                    stages.geometry = m_geometry_shader->get_source();
                }
                if (m_fragment_shader) {
                    if (definitions.has_value()) {
                        if (definitions.value() != m_fragment_shader->get_definitions() && !warned_once) {
                            NOTF_LOG_WARN("Shader Program \"{}\" uses shaders with different definitions", m_name);
                            warned_once = true;
                        }
                    } else {
                        definitions = m_fragment_shader->get_definitions();
                    }
                    stages.fragment = m_fragment_shader->get_source();
                }

                // build the multi stage shader and hope it fails...
                try {
                    // if this succeeds, get rid of the shader_ptr immediately...
                    {
                        MultiStageShader::create(m_name, std::move(stages));
                    }
                    // ... and remove the shader from the resource manager
                    ResourceManager::get_instance().get_type<MultiStageShader>().remove_all_inactive();
                    // TODO: resource manager function to delete a specific inactive resource
                }
                catch (const OpenGLError& error) {
                    error_message = error.get_message();
                }

                // if the error message is still empty, we've done all we can do
                if (error_message.empty()) { error_message = "Failed to validate the ShaderProgram"; }
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
}

void ShaderProgram::_find_uniform_blocks(const AnyShaderPtr& shader) {
    const GLuint shader_id = shader->get_id().get_value();

    GLint block_count = 0;
    NOTF_CHECK_GL(glGetProgramiv(shader_id, GL_ACTIVE_UNIFORM_BLOCKS, &block_count));
    NOTF_ASSERT(block_count >= 0);

    for (GLuint index = 0; index < static_cast<GLuint>(block_count); ++index) {
        m_uniform_blocks.emplace_back(UniformBlock(*this, shader, index));
        const auto& block = m_uniform_blocks.back();
        NOTF_LOG_TRACE("Found uniform block \"{}\" in Shader: \"{}\" with {} variables", block.get_name(),
                       shader->get_name(), block.m_variables.size());
    }
}

void ShaderProgram::_find_uniforms(const AnyShaderPtr& shader) {
    const GLuint shader_id = shader->get_id().get_value();

    GLint uniform_count = 0;
    { // discover uniforms
        NOTF_CHECK_GL(glGetProgramiv(shader_id, GL_ACTIVE_UNIFORMS, &uniform_count));
        NOTF_ASSERT(uniform_count >= 0);
        m_uniforms.reserve(m_uniforms.size() + static_cast<size_t>(uniform_count));
    }

    std::vector<GLchar> uniform_name;
    { // reserve a string long enough to hold the longest uniform name
        GLint longest_name = 0;
        NOTF_CHECK_GL(glGetProgramiv(shader_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longest_name));
        uniform_name.resize(narrow_cast<size_t>(longest_name));
    }

    // store all uniforms
    for (GLuint index = 0; index < static_cast<GLuint>(uniform_count); ++index) {
        detail::ShaderVariable new_variable;

        // type & size
        GLsizei name_length = 0;
        NOTF_CHECK_GL(glGetActiveUniform(shader_id, index, static_cast<GLsizei>(uniform_name.size()), &name_length,
                                         &new_variable.size, &new_variable.type, &uniform_name[0]));
        NOTF_ASSERT(new_variable.type);
        NOTF_ASSERT(new_variable.size);

        // name
        NOTF_ASSERT(name_length > 0);
        new_variable.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

        // location
        const GLint location = glGetUniformLocation(shader_id, new_variable.name.c_str());

        // store a free or a uniform that is part of a block
        if (location == -1) {
            for (auto& uniform_block : m_uniform_blocks) {
                for (auto& variable : uniform_block.m_variables) {
                    if (variable.index == index) {
                        NOTF_LOG_TRACE("Found uniform \"{}\" in block \"{}\" on Shader: \"{}\"", new_variable.name,
                                       uniform_block.get_name(), shader->get_name());
                        variable.type = new_variable.type;
                        variable.size = new_variable.size;
                        variable.name = new_variable.name;
                        break;
                    }
                }
            }
        } else {
            NOTF_LOG_TRACE("Found free uniform \"{}\" on Shader: \"{}\"", new_variable.name, shader->get_name());
            ShaderProgram::Uniform uniform(*this, location, shader->get_stage(), std::move(new_variable));
            m_uniforms.emplace_back(std::move(uniform));
        }
    }
}

void ShaderProgram::_find_attributes() {
    GLint attribute_count = 0;
    NOTF_CHECK_GL(glGetProgramiv(get_id().get_value(), GL_ACTIVE_ATTRIBUTES, &attribute_count));
    NOTF_ASSERT(attribute_count >= 0);
    m_attributes.reserve(static_cast<size_t>(attribute_count));

    GLint longest_attribute_length = 0;
    NOTF_CHECK_GL(glGetProgramiv(get_id().get_value(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &longest_attribute_length));
    std::vector<GLchar> uniform_name(static_cast<GLuint>(longest_attribute_length));

    for (GLuint index = 0; index < static_cast<GLuint>(attribute_count); ++index) {
        Attribute attribute;
        attribute.type = 0;
        attribute.size = 0;

        GLsizei name_length = 0;
        NOTF_CHECK_GL(glGetActiveAttrib(get_id().get_value(), index, static_cast<GLsizei>(uniform_name.size()),
                                        &name_length, &attribute.size, &attribute.type, &uniform_name[0]));
        NOTF_ASSERT(attribute.type);
        NOTF_ASSERT(attribute.size);

        NOTF_ASSERT(name_length > 0);
        attribute.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

        // some variable names are pre-defined by the language and cannot be set by the user
        if (attribute.name == "gl_VertexID") { continue; }

        attribute.index = static_cast<GLuint>(glGetAttribLocation(get_id().get_value(), attribute.name.c_str()));
        NOTF_ASSERT(attribute.index >= 0);

        NOTF_LOG_TRACE("Discovered attribute \"{}\" on ShaderProgram: \"{}\"", attribute.name, get_name());
        m_attributes.emplace_back(std::move(attribute));
    }
    m_attributes.shrink_to_fit();
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const int& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_INT || uniform.get_type() == GL_SAMPLER_2D) {
        NOTF_CHECK_GL(glProgramUniform1i(shader_id, uniform.get_location(), value));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"int\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const unsigned int& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_UNSIGNED_INT) {
        NOTF_CHECK_GL(glProgramUniform1ui(shader_id, uniform.get_location(), value));
    } else if (uniform.get_type() == GL_SAMPLER_2D) {
        NOTF_CHECK_GL(glProgramUniform1i(shader_id, uniform.get_location(), static_cast<GLint>(value)));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"uint\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const float& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_FLOAT) {
        NOTF_CHECK_GL(glProgramUniform1f(shader_id, uniform.get_location(), value));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"float\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const V2f& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_FLOAT_VEC2) {
        NOTF_CHECK_GL(glProgramUniform2fv(shader_id, uniform.get_location(), /*count*/ 1, value.as_ptr()));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"V2f\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const V4f& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_FLOAT_VEC4) {
        NOTF_CHECK_GL(glProgramUniform4fv(shader_id, uniform.get_location(), /*count*/ 1, value.as_ptr()));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"V4f\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

template<>
void ShaderProgram::_set_uniform(const GLuint shader_id, const Uniform& uniform, const M4f& value) {
    assert_is_valid(*this);
    if (uniform.get_type() == GL_FLOAT_MAT4) {
        NOTF_CHECK_GL(glProgramUniformMatrix4fv(shader_id, uniform.get_location(), /*count*/ 1,
                                                /*transpose*/ GL_FALSE, value.as_ptr()));
    } else {
        NOTF_THROW(ValueError,
                   "Uniform \"{}\" in ShaderProgram \"{}\" of type \"{}\" is not compatible with value type \"M4f\"",
                   uniform.get_name(), m_name, get_gl_type_name(uniform.get_type()));
    }
}

void ShaderProgram::_deallocate() {
    if (!m_id.is_valid()) { return; }

    NOTF_GUARD(m_context.make_current());

    NOTF_CHECK_GL(glDeleteProgramPipelines(1, &m_id.get_value()));
    m_id = ShaderProgramId::invalid();

    NOTF_LOG_TRACE("Deleted ShaderProgram: {}", m_name);
}
