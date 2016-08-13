#include "graphics/load_shaders.hpp"

#include <vector>

#include "common/log.hpp"
#include "common/system.hpp"

namespace {

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
}

namespace signal {

GLuint produce_gl_program(std::string vertex_shader_path, std::string fragment_shader_path)
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
            return GL_FALSE;
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
            return GL_FALSE;
        }
    }

    // link the program
    GLuint program = glCreateProgram();
    {
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        vertex_shader_raii.set_program(program);
        fragment_shader_raii.set_program(program);
        glLinkProgram(program);

        // check for errors
        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success) {
            log_debug << "Compiled and linked shader program with vertex shader '"
                      << basename(vertex_shader_path.c_str())
                      << "' and fragment shader '"
                      << basename(fragment_shader_path.c_str()) << "'";
        } else {
            GLint error_size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size);
            std::vector<char> error_message(static_cast<size_t>(error_size));
            glGetProgramInfoLog(program, error_size, nullptr, &error_message[0]);
            log_critical << "Failed to link shader program with vertex shader '"
                         << basename(vertex_shader_path.c_str())
                         << "' and fragment shader '"
                         << basename(fragment_shader_path.c_str()) << "'\n\t"
                         << error_message.data();
            glDeleteProgram(program);
            return GL_FALSE;
        }
    }

    return program;
}

} // namespace signal
