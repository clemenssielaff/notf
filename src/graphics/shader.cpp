#include "graphics/shader.hpp"

#include <assert.h>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "core/opengl.hpp"
#include "graphics/graphics_context.hpp"

namespace { // anonymous

/** Helper class to make sure that shaders are properly detached / removed.
 * This either happens because of errors in `build`, or because `build` has finished linking the program and the
 * objects for the individual shader stages can be savely discarded.
 */
class ShaderRAII {

public: // methods
    /** Value Constructor. */
    ShaderRAII(GLuint shader)
        : m_shader(shader), m_program(GL_FALSE) {}

    /** Destructor. */
    ~ShaderRAII()
    {
        if (m_program) {
            glDetachShader(m_program, m_shader);
        }
        glDeleteShader(m_shader);
    }

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
};

/** Returns the human readable name of the given Shader stage.
 * @param stage     Requested stage.
 */
const std::string& stage_name(const STAGE stage)
{
    static const std::string invalid  = "invalid";
    static const std::string vertex   = "vertex";
    static const std::string fragment = "fragment";
    static const std::string unknown  = "unknown";

    switch (stage) {
    case STAGE::INVALID:
        return invalid;
    case STAGE::VERTEX:
        return vertex;
    case STAGE::FRAGMENT:
        return fragment;
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
    case STAGE::INVALID:
    default:
        std::stringstream ss;
        ss << "Cannot compile " << stage_name(stage) << " shader for program \"" << name << "\"";
        const std::string msg = ss.str();
        log_critical << msg;
        throw std::runtime_error(msg);
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

        std::stringstream ss;
        ss << "Failed to compile " << stage_name(stage) << " shader for program \"" << name
           << "\"\n\t" << error_message.data();
        const std::string msg = ss.str();
        log_critical << msg;
        glDeleteShader(shader);
        throw std::runtime_error(msg);
    }
}

} // namespace anonymous

namespace notf {

std::shared_ptr<Shader> Shader::build(GraphicsContext& context,
                                      const std::string& name,
                                      const std::string& vertex_shader_source,
                                      const std::string& fragment_shader_source)
{
    if (!context.is_current()) {
        throw_runtime_error(string_format(
            "Cannot build shader \"%s\" with a context that is not current",
            name.c_str()));
    }

    // compile the shaders
    GLuint vertex_shader = compile_shader(STAGE::VERTEX, name, vertex_shader_source);
    ShaderRAII vertex_shader_raii(vertex_shader);
    GLuint fragment_shader = compile_shader(STAGE::FRAGMENT, name, fragment_shader_source);
    ShaderRAII fragment_shader_raii(fragment_shader);
    if (!(vertex_shader && fragment_shader)) {
        return {};
    }

    // link the program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    vertex_shader_raii.set_program(program);
    glAttachShader(program, fragment_shader);
    fragment_shader_raii.set_program(program);
    glLinkProgram(program);

    // check for errors
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) {
        log_trace << "Compiled and linked shader program \"" << name << "\" with vertex and fragment shader.";
    }
    else {
        GLint error_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size);
        std::vector<char> error_message(static_cast<size_t>(error_size));
        glGetProgramInfoLog(program, error_size, nullptr, &error_message[0]);

        std::stringstream ss;
        ss << "Failed to link shader program \"" << name << "\" with vertex and fragment shader:"
           << "\n\t" << error_message.data();
        const std::string msg = ss.str();
        log_critical << msg;
        glDeleteProgram(program);
        throw std::runtime_error(msg);
    }

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

void Shader::unbind()
{
    glUseProgram(GL_ZERO);
}

Shader::Shader(const GLuint id, GraphicsContext& context, const std::string name)
    : m_id(id)
    , m_graphics_context(context)
    , m_name(std::move(name))
{
}

Shader::~Shader()
{
    _deallocate();
}

void Shader::bind()
{
    if (!is_valid()) {
        throw_runtime_error(string_format(
            "Cannot bind invalid Shader \"%s\"",
            m_name.c_str()));
    }
    m_graphics_context.bind_shader(this);
}

void Shader::_deallocate()
{
    if (m_id) {
        assert(m_graphics_context.is_current());
        glDeleteProgram(m_id);
        log_trace << "Deleted Shader Program \"" << m_name << "\"";
    }
}

} // namespace notf
