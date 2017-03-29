#include "graphics/shader.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/opengl.hpp"
#include "graphics/render_context.hpp"
#include "utils/make_smart_enabler.hpp"

namespace { // anonymous

/** Helper class to make sure that shaders are properly detached / removed.
 * This either happens because of errors in `build`, or because `build` has finished linking the program and the
 * objects for the individual shader stages can be savely discarded.
 */
class ShaderRAII {

public: // methods
    /** Value Constructor. */
    ShaderRAII(GLuint shader)
        : m_shader(shader) , m_program(GL_FALSE) { }

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
        log_critical << "Cannot compile " << stage_name(stage) << " shader for program \"" << name << "\"";
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
        log_critical << "Failed to compile " << stage_name(stage) << " shader for program \"" << name
                     << "\"\n\t" << error_message.data();
        glDeleteShader(shader);
        return 0;
    }
}

} // namespace anonymous

namespace notf {

void Shader::unbind()
{
    glUseProgram(GL_ZERO);
}

std::shared_ptr<Shader> Shader::build(RenderContext* context,
                                      const std::string& name,
                                      const std::string& vertex_shader_source,
                                      const std::string& fragment_shader_source)
{
    assert(context);
    context->make_current();

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
        log_critical << "Failed to link shader program \"" << name << "\" with vertex and fragment shader:"
                     << "\n\t" << error_message.data();
        glDeleteProgram(program);
        return {};
    }
    return std::make_shared<MakeSmartEnabler<Shader>>(program, context, name);
}

Shader::Shader(const GLuint id, RenderContext *context, const std::string name)
    : m_id(id)
    , m_render_context(context)
    , m_name(std::move(name))
{
    assert(m_render_context);
}

Shader::~Shader()
{
    _deallocate();
}

bool Shader::bind()
{
    if (is_valid()) {
        m_render_context->make_current();
        m_render_context->_bind_shader(m_id);
        return true;
    }
    else {
        log_critical << "Cannot bind invalid Shader \"" << m_name << "\"";
        return false;
    }
}

void Shader::_deallocate()
{
    if (m_id) {
        m_render_context->make_current();
        glDeleteProgram(m_id);
        log_trace << "Deleted Shader Program \"" << m_name << "\"";
        m_id = 0;
        m_render_context = nullptr;
    }
}

} // namespace notf
