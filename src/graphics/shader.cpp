#include "graphics/shader.hpp"

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common/devel.hpp"
#include "common/log.hpp"
#include "common/system.hpp"

namespace { // anonymous

/*!
 * @brief Helper class to make sure that shaders are properly detached / removed.
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

namespace signal {

const std::string& Shader::stage_name(const STAGE stage)
{
    static const std::string invalid = "invalid";
    static const std::string vertex = "vertex";
    static const std::string fragment = "fragment";
    static const std::string geometry = "geometry";
    static const std::string unknown = "unknown";

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

std::shared_ptr<Shader> Shader::build(
    const std::string& shader_name,
    const std::string& vertex_shader_path,
    const std::string& fragment_shader_path,
    const std::string& geometry_shader_path)
{
    // compile the mandatory shaders
    GLuint vertex_shader = compile(STAGE::VERTEX, vertex_shader_path);
    ShaderRAII vertex_shader_raii(vertex_shader);
    GLuint fragment_shader = compile(STAGE::FRAGMENT, fragment_shader_path);
    ShaderRAII fragment_shader_raii(fragment_shader);
    if (!(vertex_shader && fragment_shader)) {
        return {};
    }

    // compile the optional geometry shader
    GLuint geometry_shader = 0;
    if (!geometry_shader_path.empty()) {
        geometry_shader = compile(STAGE::GEOMETRY, geometry_shader_path);
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
            log_trace << "Compiled and linked shader program with vertex shader '"
                      << basename(vertex_shader_path.c_str())
                      << "', fragment shader '"
                      << basename(fragment_shader_path.c_str()) << "'"
                      << "', and geometry shader '"
                      << basename(geometry_shader_path.c_str()) << "'";
        }
        else {
            log_trace << "Compiled and linked shader program with vertex shader '"
                      << basename(vertex_shader_path.c_str())
                      << "' and fragment shader '"
                      << basename(fragment_shader_path.c_str()) << "'";
        }
    }
    else {
        GLint error_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetProgramInfoLog(program, error_size, nullptr, &error_message[0]);
        if (geometry_shader) {
            log_critical << "Failed to link shader program with vertex shader '"
                         << basename(vertex_shader_path.c_str())
                         << "', fragment shader '"
                         << basename(fragment_shader_path.c_str())
                         << "', and geometry shader '"
                         << basename(geometry_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
        }
        else {
            log_critical << "Failed to link shader program with vertex shader '"
                         << basename(vertex_shader_path.c_str())
                         << "' and fragment shader '"
                         << basename(fragment_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
        }
        glDeleteProgram(program);
        return {};
    }
    return std::make_shared<MakeSharedEnabler<Shader>>(shader_name, program);
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
}

Shader& Shader::use()
{
    glUseProgram(m_id);
    return *this;
}

void Shader::set_uniform(const GLchar* name, GLfloat value)
{
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

void Shader::set_uniform(const GLchar* name, GLint value)
{
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void Shader::set_uniform(const GLchar* name, GLfloat x, GLfloat y)
{
    glUniform2f(glGetUniformLocation(m_id, name), x, y);
}

void Shader::set_uniform(const GLchar* name, const Vector2& value)
{
    glUniform2f(glGetUniformLocation(m_id, name), static_cast<GLfloat>(value.x), static_cast<GLfloat>(value.y));
}

void Shader::set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z)
{
    glUniform3f(glGetUniformLocation(m_id, name), x, y, z);
}

void Shader::set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    glUniform4f(glGetUniformLocation(m_id, name), x, y, z, w);
}

void Shader::set_uniform(const GLchar* name, const glm::vec2& value)
{
    glUniform2f(glGetUniformLocation(m_id, name), value.x, value.y);
}

void Shader::set_uniform(const GLchar* name, const glm::vec3& value)
{
    glUniform3f(glGetUniformLocation(m_id, name), value.x, value.y, value.z);
}

void Shader::set_uniform(const GLchar* name, const glm::vec4& value)
{
    glUniform4f(glGetUniformLocation(m_id, name), value.x, value.y, value.z, value.w);
}

void Shader::set_uniform(const GLchar* name, const glm::mat4& matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

GLuint Shader::compile(STAGE stage, const std::string& shader_path)
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
        log_critical << "Cannot compile geometry shader '" << shader_path
                     << "' because geometry shaders are not supprted in this version of OpenGL "
                     << "(" << glGetString(GL_VERSION) << ")";
        return 0;
#endif
    case STAGE::INVALID:
    default:
        log_critical << "Cannot compile " << stage_name(stage) << " Shader '" << shader_path << "'";
        return 0;
    }
    assert(shader);

    // compile the shader
    std::string shader_code = read_file(shader_path);
    char const* code_ptr = shader_code.c_str();
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
        log_critical << "Failed to compile " << stage_name(stage) << " shader '"
                     << basename(shader_path.c_str()) << "'\n"
                     << error_message.data();
        glDeleteShader(shader);
        return 0;
    }
}

} // namespace signal
