#include "graphics/shader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "common/log.hpp"
#include "common/system.hpp"
#include "core/glfw_wrapper.hpp"

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

Shader Shader::from_sources(
    const std::string& vertex_shader_path,
    const std::string& fragment_shader_path,
    const std::string& geometry_shader_path)
{
    // compile the vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    ShaderRAII vertex_shader_raii(vertex_shader);
    {
        std::string vertex_shader_code = read_file(vertex_shader_path);
        char const* vertex_source_ptr = vertex_shader_code.c_str();
        glShaderSource(vertex_shader, 1, &vertex_source_ptr, nullptr);
        glCompileShader(vertex_shader);

        // check for errors
        GLint success = GL_FALSE;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint error_size;
            glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &error_size);
            std::vector<char> error_message(static_cast<size_t>(error_size));
            glGetShaderInfoLog(vertex_shader, error_size, nullptr, &error_message[0]);
            log_critical << "Failed to compile vertex shader '"
                         << basename(vertex_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
            return Shader();
        }
    }

    // compile the fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    ShaderRAII fragment_shader_raii(fragment_shader);
    {
        std::string fragment_shader_code = read_file(fragment_shader_path);
        char const* fragment_source_ptr = fragment_shader_code.c_str();
        glShaderSource(fragment_shader, 1, &fragment_source_ptr, nullptr);
        glCompileShader(fragment_shader);

        // check for errors
        GLint success = GL_FALSE;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint error_size;
            glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &error_size);
            std::vector<char> error_message(static_cast<size_t>(error_size));
            glGetShaderInfoLog(fragment_shader, error_size, nullptr, &error_message[0]);
            log_critical << "Failed to compile fragment shader '"
                         << basename(fragment_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
            return Shader();
        }
    }

    GLuint geometry_shader = 0;
    ShaderRAII geometry_shader_raii(geometry_shader);
    if (!geometry_shader_path.empty()) {
#ifdef GL_GEOMETRY_SHADER
        geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);

        std::string geometry_shader_code = read_file(geometry_shader_path);
        char const* geometry_source_ptr = geometry_shader_code.c_str();
        glShaderSource(geometry_shader, 1, &geometry_source_ptr, nullptr);
        glCompileShader(geometry_shader);

        // check for errors
        GLint success = GL_FALSE;
        glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint error_size;
            glGetShaderiv(geometry_shader, GL_INFO_LOG_LENGTH, &error_size);
            std::vector<char> error_message(static_cast<size_t>(error_size));
            glGetShaderInfoLog(geometry_shader, error_size, nullptr, &error_message[0]);
            log_critical << "Failed to compile geometry shader '"
                         << basename(geometry_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
            return Shader();
        }
#else
        log_warning << "Ignored geometry shader '" << geometry_shader_path
                    << "' because geometry shaders are not supprted in this version of OpenGL "
                    << "(" << glGetString(GL_VERSION) << ")";
#endif
    }

    // link the program
    GLuint program = glCreateProgram();
    {
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
                log_debug << "Compiled and linked shader program with vertex shader '"
                          << basename(vertex_shader_path.c_str())
                          << "', fragment shader '"
                          << basename(fragment_shader_path.c_str()) << "'"
                          << "', and geometry shader '"
                          << basename(geometry_shader_path.c_str()) << "'";
            } else {
                log_debug << "Compiled and linked shader program with vertex shader '"
                          << basename(vertex_shader_path.c_str())
                          << "' and fragment shader '"
                          << basename(fragment_shader_path.c_str()) << "'";
            }

        } else {
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
            } else {
                log_critical << "Failed to link shader program with vertex shader '"
                             << basename(vertex_shader_path.c_str())
                             << "' and fragment shader '"
                             << basename(fragment_shader_path.c_str()) << "'\n\t"
                             << error_message.data();
            }
            glDeleteProgram(program);
            return Shader();
        }
    }

    return Shader(program);
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

void Shader::set_uniform(const GLchar* name, const glm::vec2& value)
{
    glUniform2f(glGetUniformLocation(m_id, name), value.x, value.y);
}

void Shader::set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z)
{
    glUniform3f(glGetUniformLocation(m_id, name), x, y, z);
}

void Shader::set_uniform(const GLchar* name, const glm::vec3& value)
{
    glUniform3f(glGetUniformLocation(m_id, name), value.x, value.y, value.z);
}

void Shader::set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    glUniform4f(glGetUniformLocation(m_id, name), x, y, z, w);
}

void Shader::set_uniform(const GLchar* name, const glm::vec4& value)
{
    glUniform4f(glGetUniformLocation(m_id, name), value.x, value.y, value.z, value.w);
}

void Shader::set_uniform(const GLchar* name, const glm::mat4& matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

} // namespace signal
