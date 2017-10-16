#include "graphics/engine/shader.hpp"

#include <assert.h>
#include <sstream>

#include "common/exception.hpp"
#include "common/float.hpp"
#include "common/log.hpp"
#include "common/system.hpp"
#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "common/xform3.hpp"
#include "core/opengl.hpp"
#include "graphics/engine/gl_utils.hpp"
#include "graphics/engine/graphics_context.hpp"

namespace { // anonymous

/** Helper class to make sure that shaders are properly detached / removed.
 * This either happens because of errors in `build`, or because `build` has finished linking the program and the
 * objects for the individual shader stages can be savely discarded.
 */
class ShaderRAII {

public: // methods
    /** Default constructor. */
    ShaderRAII()
        : m_shader(0), m_program(0) {}

    /** Value constructor. */
    ShaderRAII(GLuint shader)
        : m_shader(shader), m_program(0) {}

    /** Destructor. */
    ~ShaderRAII()
    {
        if (m_program) {
            glDetachShader(m_program, m_shader);
        }
        if (m_shader) {
            glDeleteShader(m_shader);
        }
    }

    /** Updates the managed shader. */
    void set_shader(GLuint shader) { m_shader = shader; }

    /** If the Shader has been attached to a program, it needs to be detached first before it is removed. */
    void set_program(GLuint program) { m_program = program; }

private: // fields
    /** ID of the managed OpenGL shader. */
    GLuint m_shader;

    /** ID of OpenGL program, that the shader is attached to. */
    GLuint m_program;
};

/** Shader stages. */
enum class STAGE : unsigned char {
    INVALID = 0,
    VERTEX,
    FRAGMENT,
    GEOMETRY,
};

/** Returns the human readable name of the given Shader stage.
 * @param stage     Requested stage.
 */
const std::string& stage_name(const STAGE stage)
{
    static const std::string invalid  = "invalid";
    static const std::string vertex   = "vertex";
    static const std::string fragment = "fragment";
    static const std::string geometry = "geometry";
    static const std::string unknown  = "unknown";

    switch (stage) {
    case STAGE::INVALID:
        return invalid;
    case STAGE::VERTEX:
        return vertex;
    case STAGE::FRAGMENT:
        return fragment;
    case STAGE::GEOMETRY:
        return geometry;
    }
    return unknown;
}

/** Compiles a single shader stage from a given source.
 * @param stage     Stage of the Shader represented by the source.
 * @param name      Name of the Shader program (for error messages).
 * @param source    Source to compile.
 * @return OpenGL ID of the shader - is 0 on error.
 */
GLuint compile_shader(STAGE stage, const std::string& name, const std::string& source)
{
    // create the OpenGL shader
    GLuint shader = 0;
    switch (stage) {
    case STAGE::VERTEX:
        shader = glCreateShader(GL_VERTEX_SHADER);
        break;
    case STAGE::FRAGMENT:
        shader = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    case STAGE::GEOMETRY:
        shader = glCreateShader(GL_GEOMETRY_SHADER);
        break;
    case STAGE::INVALID:
    default:
        std::stringstream ss;
        ss << "Cannot compile " << stage_name(stage) << " shader for program \"" << name << "\"";
        throw_runtime_error(ss.str());
    }
    if (!shader) {
        std::stringstream ss;
        ss << "Failed to create " << stage_name(stage) << " shader object for for program \"" << name << "\"";
        throw_runtime_error(ss.str());
    }

    // compile the shader
    char const* code_ptr = source.c_str();
    glShaderSource(shader, /*count*/ 1, &code_ptr, /*length*/ 0);
    glCompileShader(shader);

    // check for errors
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint error_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetShaderInfoLog(shader, error_size, /*length*/ 0, &error_message[0]);
        glDeleteShader(shader);

        std::stringstream ss;
        ss << "Failed to compile " << stage_name(stage) << " stage for shader \"" << name
           << "\"\n\t" << error_message.data();
        throw_runtime_error(ss.str());
    }
    return shader;
}

} // namespace anonymous

namespace notf {

Shader::Scope::Scope(Shader* shader)
    : m_shader(shader)
{
    m_shader->m_graphics_context.push_shader(m_shader->shared_from_this());
}

Shader::Scope::~Scope()
{
    if (m_shader) {
        m_shader->m_graphics_context.pop_shader();
    }
}

std::shared_ptr<Shader> Shader::load(GraphicsContext& context, const std::string& name,
                                     const std::string& vertex_shader_file,
                                     const std::string& fragment_shader_file,
                                     const std::string& geometry_shader_file)
{
    const std::string vertex_shader_source   = load_file(vertex_shader_file);
    const std::string fragment_shader_source = load_file(fragment_shader_file);
    std::string geometry_shader_source;
    if (!geometry_shader_file.empty()) {
        geometry_shader_source = load_file(geometry_shader_file);
    }
    return Shader::build(context, name, vertex_shader_source, fragment_shader_source, geometry_shader_source);
}

std::shared_ptr<Shader> Shader::build(GraphicsContext& context,
                                      const std::string& name,
                                      const std::string& vertex_shader_source,
                                      const std::string& fragment_shader_source,
                                      const std::string& geometry_shader_source)
{
    if (!context.is_current()) {
        throw_runtime_error(string_format(
            "Cannot build shader \"%s\" with a context that is not current",
            name.c_str()));
    }

    // compile the shaders
    GLuint vertex_shader = compile_shader(STAGE::VERTEX, name, vertex_shader_source);
    ShaderRAII vertex_shader_raii(vertex_shader);
    if (!vertex_shader) {
        return {};
    }
    GLuint fragment_shader = compile_shader(STAGE::FRAGMENT, name, fragment_shader_source);
    ShaderRAII fragment_shader_raii(fragment_shader);
    if (!fragment_shader) {
        return {};
    }
    GLuint geometry_shader = 0;
    ShaderRAII geometry_shader_raii;
    if (!geometry_shader_source.empty()) {
        geometry_shader = compile_shader(STAGE::GEOMETRY, name, geometry_shader_source);
        if (!geometry_shader) {
            return {};
        }
        geometry_shader_raii.set_shader(geometry_shader);
    }

    // link the program
    GLuint program = glCreateProgram();
    if (!program) {
        std::stringstream ss;
        ss << "Failed to create program object for shader \"" << name << "\"";
        throw_runtime_error(ss.str());
    }

    glAttachShader(program, vertex_shader);
    vertex_shader_raii.set_program(program);

    glAttachShader(program, fragment_shader);
    fragment_shader_raii.set_program(program);

    if (geometry_shader) {
        glAttachShader(program, geometry_shader);
        geometry_shader_raii.set_program(program);
    }

    glLinkProgram(program);

    // check for errors
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint error_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetProgramInfoLog(program, error_size, /*length*/ 0, &error_message[0]);
        glDeleteProgram(program);

        std::stringstream ss;
        ss << "Failed to link shader program \"" << name << "\":\n"
           << error_message.data();
        throw_runtime_error(ss.str());
    }
    log_trace << "Compiled and linked shader program \"" << name << "\".";

// create the Shader object
#ifdef _DEBUG
    std::shared_ptr<Shader> shader(new Shader(program, context, name));
#else
    struct make_shared_enabler : public Shader {
        make_shared_enabler(const GLuint id, GraphicsContext& context, const std::string name)
            : Shader(id, context, name) {}
    };
    std::shared_ptr<Shader> shader = std::make_shared<make_shared_enabler>(program, context, name);
#endif
    context.m_shaders.emplace_back(shader);
    return shader;
}

Shader::Shader(const GLuint id, GraphicsContext& context, const std::string name)
    : m_id(id)
    , m_graphics_context(context)
    , m_name(std::move(name))
    , m_uniforms()
    , m_attributes()
{
    // allocate a string long enough to hold all expected attribute- and uniform names
    std::vector<GLchar> variable_name;
    {
        GLint longest_attribute_length = 0;
        glGetProgramiv(m_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &longest_attribute_length);

        GLint longest_uniform_length = 0;
        glGetProgramiv(m_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longest_uniform_length);

        variable_name.resize(static_cast<size_t>(max(longest_attribute_length, longest_uniform_length)));
    }

    { // discover uniforms
        GLint uniform_count = 0;
        glGetProgramiv(m_id, GL_ACTIVE_UNIFORMS, &uniform_count);
        assert(uniform_count >= 0);
        m_uniforms.reserve(static_cast<size_t>(uniform_count));

        for (GLuint index = 0; index < static_cast<GLuint>(uniform_count); ++index) {
            Variable variable;
            variable.index = index;
            variable.type  = 0;
            variable.size  = 0;

            GLsizei name_length = 0;
            glGetActiveUniform(m_id, index, static_cast<GLsizei>(variable_name.size()),
                               &name_length, &variable.size, &variable.type, &variable_name[0]);
            assert(variable.type);
            assert(variable.size);

            assert(name_length > 0);
            variable.name = std::string(&variable_name[0], static_cast<size_t>(name_length));

            variable.location = glGetUniformLocation(m_id, variable.name.c_str());
            if(variable.location == -1){
                log_warning << "Skipping uniform \"" << variable.name << "\" on shader: \"" << m_name << "\"";
                // TODO: this is skipping uniforms in a named uniform block
                continue;
            }

            log_trace << "Discovered uniform \"" << variable.name << "\" on shader: \"" << m_name << "\"";
            m_uniforms.emplace_back(std::move(variable));
        }
        m_uniforms.shrink_to_fit();
    }

    { // discover attributes
        GLint attribute_count = 0;
        glGetProgramiv(m_id, GL_ACTIVE_ATTRIBUTES, &attribute_count);
        assert(attribute_count >= 0);
        m_attributes.reserve(static_cast<size_t>(attribute_count));

        for (GLuint index = 0; index < static_cast<GLuint>(attribute_count); ++index) {

            Variable variable;
            variable.index = index;
            variable.type  = 0;
            variable.size  = 0;

            GLsizei name_length = 0;
            glGetActiveAttrib(m_id, index, static_cast<GLsizei>(variable_name.size()),
                              &name_length, &variable.size, &variable.type, &variable_name[0]);
            assert(variable.type);
            assert(variable.size);

            assert(name_length > 0);
            variable.name = std::string(&variable_name[0], static_cast<size_t>(name_length));

            variable.location = glGetAttribLocation(m_id, variable.name.c_str());
            assert(variable.location >= 0);

            log_trace << "Discovered attribute \"" << variable.name << "\" on shader: \"" << m_name << "\"";
            m_attributes.emplace_back(std::move(variable));
        }
        m_attributes.shrink_to_fit();
    }
}

Shader::~Shader()
{
    _deallocate();
}

GLuint Shader::attribute(const std::string& name) const
{
    for (const Variable& variable : m_attributes) {
        if (variable.name == name) {
            return static_cast<GLuint>(variable.location);
        }
    }
    throw_runtime_error(string_format("No attribute named \"%s\" in shader \"%s\"", name.c_str(), m_name.c_str()));
}

#ifdef _DEBUG
bool Shader::validate_now() const
{
    glValidateProgram(m_id);

    GLint status = GL_FALSE;
    glGetProgramiv(m_id, GL_VALIDATE_STATUS, &status);

    GLint message_size;
    glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &message_size);
    std::vector<char> message(static_cast<size_t>(message_size));
    glGetProgramInfoLog(m_id, message_size, /*length*/ 0, &message[0]);

    log_trace << "Validation of shader \"" << m_name << "\" " << (status ? "succeeded" : "failed:\n")
              << message.data();
    return status == GL_TRUE;
}
#endif

const Shader::Variable& Shader::_uniform(const std::string& name) const
{
    auto it = std::find_if(
        std::begin(m_uniforms), std::end(m_uniforms),
        [&](const Variable& var) -> bool {
            return var.name == name;
        });
    if (it == std::end(m_uniforms)) {
        throw_runtime_error(string_format("No uniform named \"%s\" in shader \"%s\"", name.c_str(), m_name.c_str()));
    }
    return *it;
}

void Shader::_deallocate()
{
    if (m_id) {
        assert(m_graphics_context.is_current());
        glDeleteProgram(m_id);
        log_trace << "Deleted Shader Program \"" << m_name << "\"";
    }
    m_id = 0;
}

template <>
void Shader::set_uniform(const std::string& name, const int& value)
{
    const Variable& uniform = _uniform(name);
    Scope _(this);
//    if (uniform.type == GL_INT) {
        glUniform1i(uniform.location, value);
//    }
//    else {
//        m_graphics_context.pop_shader();
//        throw_runtime_error(string_format(
//                                "Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"int\"",
//                                name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
//    }
}

template <>
void Shader::set_uniform(const std::string& name, const float& value)
{
    const Variable& uniform = _uniform(name);
    Scope _(this);
    if (uniform.type == GL_FLOAT) {
        glUniform1f(uniform.location, value);
    }
    else {
        m_graphics_context.pop_shader();
        throw_runtime_error(string_format(
            "Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"float\"",
            name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template <>
void Shader::set_uniform(const std::string& name, const Vector2f& value)
{
    const Variable& uniform = _uniform(name);
    Scope _(this);
    if (uniform.type == GL_FLOAT_VEC2) {
        glUniform2fv(uniform.location, /*count*/ 1, value.as_ptr());
    }
    else {
        throw_runtime_error(string_format(
            "Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"Vector2f\"",
            name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template <>
void Shader::set_uniform(const std::string& name, const Vector4f& value)
{
    const Variable& uniform = _uniform(name);
    Scope _(this);
    if (uniform.type == GL_FLOAT_VEC4) {
        glUniform4fv(uniform.location, /*count*/ 1, value.as_ptr());
    }
    else {
        throw_runtime_error(string_format(
            "Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"Vector2f\"",
            name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

template <>
void Shader::set_uniform(const std::string& name, const Xform3f& value)
{
    const Variable& uniform = _uniform(name);
    Scope _(this);
    if (uniform.type == GL_FLOAT_MAT4) {
        glUniformMatrix4fv(uniform.location, /*count*/ 1, GL_FALSE, value.as_ptr());
    }
    else {
        throw_runtime_error(string_format(
            "Uniform \"%s\" in shader \"%s\" of type \"%s\" is not compatible with value type \"Vector2f\"",
            name.c_str(), m_name.c_str(), gl_type_name(uniform.type).c_str()));
    }
}

} // namespace notf
