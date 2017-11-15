#include "graphics/shader.hpp"

#include <algorithm>
#include <assert.h>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/system.hpp"
#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "utils/narrow_cast.hpp"

namespace { // anonymous

using namespace notf;

/// @brief Compiles a single Shader stage from a given source.
/// @param stage     Stage of the Shader produced by the source.
/// @param name      Name of the Shader program (for error messages).
/// @param source    Source to compile.
/// @return OpenGL ID of the shader.
GLuint compile_shader(const Pipeline::Stage stage, const std::string& name, const char* source)
{
    static const std::string vertex   = "vertex";
    static const std::string fragment = "fragment";
    static const std::string geometry = "geometry";

    // create the OpenGL shader
    GLuint shader                 = 0;
    const std::string* stage_name = nullptr;
    switch (stage) {
    case Pipeline::Stage::VERTEX:
        shader     = glCreateShader(GL_VERTEX_SHADER);
        stage_name = &vertex;
        break;
    case Pipeline::Stage::GEOMETRY:
        shader     = glCreateShader(GL_GEOMETRY_SHADER);
        stage_name = &geometry;
        break;
    case Pipeline::Stage::FRAGMENT:
        shader     = glCreateShader(GL_FRAGMENT_SHADER);
        stage_name = &fragment;
        break;
    }
    assert(stage_name);

    if (!shader) {
        std::stringstream ss;
        ss << "Failed to create " << *stage_name << " shader object for for program \"" << name << "\"";
        throw_runtime_error(ss.str());
    }

    // compile the shader
    gl_check(glShaderSource(shader, /*count*/ 1, &source, /*length*/ 0));
    gl_check(glCompileShader(shader));

    // check for errors
    GLint success = GL_FALSE;
    gl_check(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
    if (!success) {
        GLint error_size;
        gl_check(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_size));
        std::vector<char> error_message(static_cast<size_t>(error_size));
        gl_check(glGetShaderInfoLog(shader, error_size, /*length*/ 0, &error_message[0]));
        gl_check(glDeleteShader(shader));

        std::stringstream ss;
        ss << "Failed to compile " << *stage_name << " stage for shader \"" << name << "\"\n\t" << error_message.data();
        throw_runtime_error(ss.str());
    }

    assert(shader);
    return shader;
}

/// @brief Get the size of the longest uniform name in a program.
size_t longest_uniform_length(const GLuint program)
{
    GLint result = 0;
    gl_check(glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &result));
    return narrow_cast<size_t>(result);
}

/// @brief Get the size of the longest attribute name in a program.
size_t longest_attribute_length(const GLuint program)
{
    GLint result = 0;
    gl_check(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &result));
    return narrow_cast<size_t>(result);
}

} // namespace

// ===================================================================================================================//

namespace notf {

ShaderPtr Shader::_build(GraphicsContextPtr& context, std::string name, const Pipeline::Stage stage, const char* source)
{
    // check if the name is unique
    if (context->has_shader(name)) {
        std::stringstream ss;
        ss << "Cannot create a new Shader with existing name \"" << name << "\"";
        throw_runtime_error(ss.str());
    }

    gl_clear_errors();

    // create the program
    // We don't use `glCreateShaderProgramv` so we could pass additional pre-link parameters.
    // For details, see:
    //     https://www.khronos.org/opengl/wiki/Interface_Matching#Separate_programs
    GLuint program = glCreateProgram();
    if (!program) {
        std::stringstream ss;
        ss << "Failed to create program object for shader \"" << name << "\"";
        throw_runtime_error(ss.str());
    }
    gl_check(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE));

    // attach the compiled shader
    GLuint shader = compile_shader(stage, name, source);
    gl_check(glAttachShader(program, shader));
    gl_check(glLinkProgram(program));
    gl_check(glDetachShader(program, shader));
    gl_check(glDeleteShader(shader));

    { // check for errors
        GLint success = GL_FALSE;
        gl_check(glGetProgramiv(program, GL_LINK_STATUS, &success));
        if (!success) {
            GLint error_size;
            gl_check(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size));
            std::vector<char> error_message(static_cast<size_t>(error_size));
            gl_check(glGetProgramInfoLog(program, error_size, /*length*/ 0, &error_message[0]));
            gl_check(glDeleteProgram(program));

            std::stringstream ss;
            ss << "Failed to link shader program \"" << name << "\":\n" << error_message.data();
            throw_runtime_error(ss.str());
        }
    }
    log_trace << "Compiled and linked shader program \"" << name << "\".";

    // create the Shader object
    ShaderPtr result;
#ifdef NOTF_DEBUG
    switch (stage) {
    case Pipeline::Stage::VERTEX:
        result = ShaderPtr(new VertexShader(context, program, name));
        break;
    case Pipeline::Stage::GEOMETRY:
        result = ShaderPtr(new GeometryShader(context, program, name));
        break;
    case Pipeline::Stage::FRAGMENT:
        result = ShaderPtr(new FragmentShader(context, program, name));
        break;
    }
#else
    struct vertex_make_shared_enabler : public VertexShader {
        vertex_make_shared_enabler(const GLuint id, GraphicsContextPtr& context, std::string name)
            : VertexShader(context, id, std::move(name))
        {}
    };
    struct geometry_make_shared_enabler : public GeometryShader {
        geometry_make_shared_enabler(const GLuint id, GraphicsContextPtr& context, std::string name)
            : GeometryShader(context, id, std::move(name))
        {}
    };
    struct fragment_make_shared_enabler : public FragmentShader {
        fragment_make_shared_enabler(const GLuint id, GraphicsContextPtr& context, std::string name)
            : FragmentShader(context, id, std::move(name))
        {}
    };
    switch (stage) {
    case Pipeline::Stage::VERTEX:
        result = std::make_shared<vertex_make_shared_enabler>(program, context, name);
        break;
    case Pipeline::Stage::GEOMETRY:
        result = std::make_shared<geometry_make_shared_enabler>(program, context, name);
        break;
    case Pipeline::Stage::FRAGMENT:
        result = std::make_shared<fragment_make_shared_enabler>(program, context, name);
        break;
    }
#endif
    assert(result);

    context->m_shaders.emplace(std::move(name), result);
    return result;
}

ShaderPtr
Shader::_load(GraphicsContextPtr& context, std::string name, const Pipeline::Stage stage, const std::string& file)
{
    const std::string shader_source = load_file(file);
    return Shader::_build(context, name, stage, shader_source.c_str());
}

Shader::Shader(GraphicsContextPtr& context, const GLuint id, std::string name)
    : m_graphics_context(*context), m_id(id), m_name(std::move(name)), m_uniforms()
{
    // discover uniforms
    GLint uniform_count = 0;
    gl_check(glGetProgramiv(m_id, GL_ACTIVE_UNIFORMS, &uniform_count));
    assert(uniform_count >= 0);
    m_uniforms.reserve(static_cast<size_t>(uniform_count));

    std::vector<GLchar> uniform_name(longest_uniform_length(m_id));
    for (GLuint index = 0; index < static_cast<GLuint>(uniform_count); ++index) {
        Variable variable;
        variable.type = 0;
        variable.size = 0;

        GLsizei name_length = 0;
        gl_check(glGetActiveUniform(m_id, index, static_cast<GLsizei>(uniform_name.size()), &name_length,
                                    &variable.size, &variable.type, &uniform_name[0]));
        assert(variable.type);
        assert(variable.size);

        assert(name_length > 0);
        variable.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

        variable.location = glGetUniformLocation(m_id, variable.name.c_str());
        if (variable.location == -1) {
            log_warning << "Skipping uniform \"" << variable.name << "\" on shader: \"" << m_name << "\"";
            // TODO: this is skipping uniforms in a named uniform block
            continue;
        }

        log_trace << "Discovered uniform \"" << variable.name << "\" on shader: \"" << m_name << "\"";
        m_uniforms.emplace_back(std::move(variable));
    }
    m_uniforms.shrink_to_fit();
}

Shader::~Shader() { _deallocate(); }

#ifdef NOTF_DEBUG
bool Shader::validate_now() const
{
    gl_check(glValidateProgram(m_id));

    GLint status = GL_FALSE;
    gl_check(glGetProgramiv(m_id, GL_VALIDATE_STATUS, &status));

    GLint message_size;
    gl_check(glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &message_size));
    std::vector<char> message(static_cast<size_t>(message_size));
    gl_check(glGetProgramInfoLog(m_id, message_size, /*length*/ 0, &message[0]));

    log_trace << "Validation of shader \"" << m_name << "\" " << (status ? "succeeded" : "failed:\n") << message.data();
    return status == GL_TRUE;
}
#endif

const Shader::Variable& Shader::_uniform(const std::string& name) const
{
    auto it = std::find_if(std::begin(m_uniforms), std::end(m_uniforms),
                           [&](const Variable& var) -> bool { return var.name == name; });
    if (it == std::end(m_uniforms)) {
        throw_runtime_error(string_format("No uniform named \"%s\" in shader \"%s\"", name.c_str(), m_name.c_str()));
    }
    return *it;
}

void Shader::_deallocate()
{
    if (m_id) {
        gl_check(glDeleteProgram(m_id));
        log_trace << "Deleted Shader Program \"" << m_name << "\"";
    }
    m_id = 0;
}

template<>
void Shader::set_uniform(const std::string& name, const int& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_INT || uniform.type == GL_SAMPLER_2D) {
        gl_check(glProgramUniform1i(m_id, uniform.location, value));
    }
    else {
        throw_runtime_error(
            string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"int\"",
                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const unsigned int& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_UNSIGNED_INT) {
        gl_check(glProgramUniform1ui(m_id, uniform.location, value));
    }
    else if (uniform.type == GL_SAMPLER_2D) {
        gl_check(glProgramUniform1i(m_id, uniform.location, static_cast<GLint>(value)));
    }
    else {
        throw_runtime_error(string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value "
                                          "type \"unsigned int\"",
                                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const float& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT) {
        gl_check(glProgramUniform1f(m_id, uniform.location, value));
    }
    else {
        throw_runtime_error(
            string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"float\"",
                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const Vector2f& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_VEC2) {
        gl_check(glProgramUniform2fv(m_id, uniform.location, /*count*/ 1, value.as_ptr()));
    }
    else {
        throw_runtime_error(string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value "
                                          "type \"Vector2f\"",
                                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const Vector4f& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_VEC4) {
        gl_check(glProgramUniform4fv(m_id, uniform.location, /*count*/ 1, value.as_ptr()));
    }
    else {
        throw_runtime_error(string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value "
                                          "type \"Vector2f\"",
                                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const Matrix4f& value)
{
    const Variable& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_MAT4) {
        gl_check(
            glProgramUniformMatrix4fv(m_id, uniform.location, /*count*/ 1, /*transpose*/ GL_FALSE, value.as_ptr()));
    }
    else {
        throw_runtime_error(string_format("Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value "
                                          "type \"Vector2f\"",
                                          name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

// ===================================================================================================================//

VertexShader::VertexShader(GraphicsContextPtr& context, const GLuint program, std::string shader_name)
    : Shader(context, program, std::move(shader_name)), m_attributes()
{
    // discover attributes
    GLint attribute_count = 0;
    gl_check(glGetProgramiv(id(), GL_ACTIVE_ATTRIBUTES, &attribute_count));
    assert(attribute_count >= 0);
    m_attributes.reserve(static_cast<size_t>(attribute_count));

    std::vector<GLchar> uniform_name(longest_attribute_length(id()));
    for (GLuint index = 0; index < static_cast<GLuint>(attribute_count); ++index) {
        Variable variable;
        variable.type = 0;
        variable.size = 0;

        GLsizei name_length = 0;
        gl_check(glGetActiveAttrib(id(), index, static_cast<GLsizei>(uniform_name.size()), &name_length, &variable.size,
                                   &variable.type, &uniform_name[0]));
        assert(variable.type);
        assert(variable.size);

        assert(name_length > 0);
        variable.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

        variable.location = glGetAttribLocation(id(), variable.name.c_str());
        assert(variable.location >= 0);

        log_trace << "Discovered attribute \"" << variable.name << "\" on shader: \"" << name() << "\"";
        m_attributes.emplace_back(std::move(variable));
    }
    m_attributes.shrink_to_fit();
}

GLuint VertexShader::attribute(const std::string& attribute_name) const
{
    for (const Variable& variable : m_attributes) {
        if (variable.name == attribute_name) {
            return static_cast<GLuint>(variable.location);
        }
    }
    throw_runtime_error(
        string_format("No attribute named \"%s\" in shader \"%s\"", attribute_name.c_str(), name().c_str()));
}

// ===================================================================================================================//

GeometryShader::GeometryShader(GraphicsContextPtr& context, const GLuint program, std::string shader_name)
    : Shader(context, program, std::move(shader_name))
{}

// ===================================================================================================================//

FragmentShader::FragmentShader(GraphicsContextPtr& context, const GLuint program, std::string shader_name)
    : Shader(context, program, std::move(shader_name))
{}

} // namespace notf
