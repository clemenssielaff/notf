#include "graphics2/shader.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/glfw_wrapper.hpp"
namespace { // anonymous

/** Helper class to make sure that shaders are properly detached / removed.
 * This either happens because of errors in `build`, or because `build` has finished linking the program and the
 * objects for the individual shader stages can be savely discarded.
 */
class ShaderRAII {
public:
    ShaderRAII(GLuint shader)
        : m_shader(shader)
        , m_program(GL_FALSE)
    {
    }

    ~ShaderRAII()
    {
        if (m_program) {
            glDetachShader(m_program, m_shader);
        }
        glDeleteShader(m_shader);
    }

    void set_program(GLuint program) { m_program = program; }

private:
    GLuint m_shader;
    GLuint m_program;
};

} // namespace anonymous

namespace notf {

const std::string& Shader::_stage_name(const STAGE stage)
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

Shader Shader::build(
    const std::string& name,
    const std::string& vertex_shader_source,
    const std::string& fragment_shader_source,
    const std::string& geometry_shader_source)
{
    // compile the mandatory shaders
    GLuint vertex_shader = _compile(STAGE::VERTEX, name, vertex_shader_source);
    ShaderRAII vertex_shader_raii(vertex_shader);
    GLuint fragment_shader = _compile(STAGE::FRAGMENT, name, fragment_shader_source);
    ShaderRAII fragment_shader_raii(fragment_shader);
    if (!(vertex_shader && fragment_shader)) {
        return {};
    }

    // compile the optional geometry shader
    GLuint geometry_shader = 0;
    if (!geometry_shader_source.empty()) {
        geometry_shader = _compile(STAGE::GEOMETRY, name, geometry_shader_source);
        if (!geometry_shader) {
            return {};
        }
    }
    ShaderRAII geometry_shader_raii(geometry_shader);

    // link the program
    GLuint program = glCreateProgram();
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
    if (success) {
        if (geometry_shader) {
            log_trace << "Compiled and linked shader program \""
                      << name
                      << "\" with vertex, fragment and geometry shader.";
        }
        else {
            log_trace << "Compiled and linked shader program \""
                      << name
                      << "\" with vertex and fragment shader.";
        }
    }
    else {
        GLint error_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetProgramInfoLog(program, error_size, nullptr, &error_message[0]);
        if (geometry_shader) {
            log_critical << "Failed to link shader program \""
                         << name
                         << "\" with vertex, fragment and geometry shader:\n\t"
                         << error_message.data();
        }
        else {
            log_critical << "Failed to link shader program \""
                         << name
                         << "\" with vertex and fragment shader:\n\t"
                         << error_message.data();
        }
        glDeleteProgram(program);
        return {};
    }
    return Shader(name, program);
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
    log_trace << "Deleted Shader Program \"" << m_name << "\"";
}

GLuint Shader::_compile(STAGE stage, const std::string& name, const std::string& source)
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
#ifdef GL_GEOMETRY_SHADER
        shader = glCreateShader(GL_GEOMETRY_SHADER);
        break;
#else
        log_critical << "Cannot compile geometry shader for '" << name
                     << "' because geometry shaders are not supprted in this version of OpenGL "
                     << "(" << glGetString(GL_VERSION) << ")";
        return 0;
#endif
    case STAGE::INVALID:
    default:
        log_critical << "Cannot compile " << _stage_name(stage) << " shader for program \"" << name << "\"";
        return 0;
    }
    assert(shader);

    // compile the shader
    char const* code_ptr = source.c_str();
    glShaderSource(shader, 1, &code_ptr, nullptr);
    glCompileShader(shader);

    // check for errors
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) {
        return shader;
    }
    else {
        GLint error_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetShaderInfoLog(shader, error_size, nullptr, &error_message[0]);
        log_critical << "Failed to compile " << _stage_name(stage) << " shader for program \""
                     << name << "\"\n\t"
                     << error_message.data();
        glDeleteShader(shader);
        return 0;
    }
}

} // namespace notf
