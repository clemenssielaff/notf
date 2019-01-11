#include "notf/graphic/shader.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/common/matrix4.hpp"
#include "notf/common/vector2.hpp"
#include "notf/common/vector4.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/gl_utils.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/opengl.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

/// Compiles a single Shader stage from a given source.
/// @param program_name Name of the Shader program (for error messages).
/// @param stage        Shader stage produced by the source.
/// @param source       Source to compile.
/// @return OpenGL ID of the Shader stage.
GLuint compile_stage(const std::string& program_name, const Shader::Stage::Flag stage, const char* source) {
    static const char* vertex = "vertex";
    static const char* tess_ctrl = "tesselation-control";
    static const char* tess_eval = "tesselation-evaluation";
    static const char* geometry = "geometry";
    static const char* fragment = "fragment";
    static const char* compute = "compute";

    if (!source) { return 0; }

    // create the OpenGL Shader
    GLuint shader = 0;
    const char* stage_name = nullptr;
    switch (stage) {
    case Shader::Stage::VERTEX:
        shader = glCreateShader(GL_VERTEX_SHADER);
        stage_name = vertex;
        break;
    case Shader::Stage::TESS_CONTROL:
        shader = glCreateShader(GL_TESS_CONTROL_SHADER);
        stage_name = tess_ctrl;
        break;
    case Shader::Stage::TESS_EVALUATION:
        shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        stage_name = tess_eval;
        break;
    case Shader::Stage::GEOMETRY:
        shader = glCreateShader(GL_GEOMETRY_SHADER);
        stage_name = geometry;
        break;
    case Shader::Stage::FRAGMENT:
        shader = glCreateShader(GL_FRAGMENT_SHADER);
        stage_name = fragment;
        break;
    case Shader::Stage::COMPUTE:
        shader = glCreateShader(GL_COMPUTE_SHADER);
        stage_name = compute;
        break;
    }
    NOTF_ASSERT(stage_name);

    if (!shader) {
        NOTF_THROW(OpenGLError, "Failed to create {} Shader object for for program \"{}\"", stage_name, program_name);
    }

    // compile the shader
    NOTF_CHECK_GL(glShaderSource(shader, /*count*/ 1, &source, /*length*/ nullptr));
    NOTF_CHECK_GL(glCompileShader(shader));

    // check for errors
    GLint success = GL_FALSE;
    NOTF_CHECK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
    if (!success) {
        GLint error_size;
        NOTF_CHECK_GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_size));
        std::vector<char> error_message(static_cast<size_t>(error_size));
        NOTF_CHECK_GL(glGetShaderInfoLog(shader, error_size, /*length*/ nullptr, &error_message[0]));
        NOTF_CHECK_GL(glDeleteShader(shader));

        NOTF_THROW(OpenGLError, "Failed to compile {} stage for Shader \"{}\"\n\t{}", stage_name, program_name,
                   error_message.data());
    }

    NOTF_ASSERT(shader);
    return shader;
}

/// Get the size of the longest uniform name in a program.
size_t longest_uniform_length(const GLuint program) {
    GLint result = 0;
    NOTF_CHECK_GL(glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &result));
    return narrow_cast<size_t>(result);
}

/// Finds the index in a given GLSL source string where custom `#defines` can be injected.
size_t find_injection_index(const std::string& source) {
    static const std::regex version_regex(R"==(\n?\s*#version\s*\d{3}\s*es[ \t]*\n)==");
    static const std::regex extensions_regex(R"==(\n\s*#extension\s*\w*\s*:\s*(?:require|enable|warn|disable)\s*\n)==");

    size_t injection_index = std::string::npos;

    std::smatch extensions;
    std::regex_search(source, extensions, extensions_regex);
    if (!extensions.empty()) {
        // if the Shader contains one or more #extension strings, we have to inject the #defines after those
        injection_index = 0;
        std::string remainder;
        do {
            remainder = extensions.suffix();
            injection_index += narrow_cast<size_t>(extensions.position(extensions.size() - 1)
                                                   + extensions.length(extensions.size() - 1));
            std::regex_search(remainder, extensions, extensions_regex);
        } while (!extensions.empty());
    } else {
        // otherwise, look for the mandatory #version string
        std::smatch version;
        std::regex_search(source, version, version_regex);
        if (!version.empty()) {
            injection_index
                = narrow_cast<size_t>(version.position(version.size() - 1) + version.length(version.size() - 1));
        }
    }

    return injection_index;
}

/// Builds a string out of Shader Definitions.
std::string build_defines(const Shader::Defines& defines) {
    std::stringstream ss;
    for (const std::pair<std::string, std::string>& define : defines) {
        ss << "#define " << define.first << " " << define.second << "\n";
    }
    return ss.str();
}

namespace detail {

std::string build_glsl_header() {
    std::string result = "\n//==== notf header ========================================\n\n";

    const auto& extensions = TheGraphicsSystem::get_extensions();

    { // pragmas first ...
        bool any_pragma = false;
#ifdef NOTF_DEBUG
        any_pragma = true;
        result += "#pragma debug(on)\n";
#endif
        if (any_pragma) { result += "\n"; }
    }

    { // ... then extensions ...
        bool any_extensions = false;
        if (extensions.gpu_shader5) {
            result += "#extension GL_EXT_gpu_shader5 : enable\n";
            any_extensions = true;
        }
        if (any_extensions) { result += "\n"; }
    }

    { // ... and defines last
        bool any_defines = false;
        if (!extensions.gpu_shader5) {
            result += "#define int8_t    int \n"
                      "#define int16_t   int \n"
                      "#define int32_t   int \n"
                      "#define int64_t   int \n"
                      "#define uint8_t   uint \n"
                      "#define uint16_t  uint \n"
                      "#define uint32_t  uint \n"
                      "#define uint64_t  uint \n"
                      "#define float16_t float \n"
                      "#define float32_t float \n"
                      "#define float64_t double \n\n"
                      "#define i8vec2   ivec2 \n"
                      "#define i16vec2  ivec2 \n"
                      "#define i32vec2  ivec2 \n"
                      "#define i64vec2  ivec2 \n"
                      "#define u8vec2   uvec2 \n"
                      "#define u16vec2  uvec2 \n"
                      "#define u32vec2  uvec2 \n"
                      "#define u64vec2  uvec2 \n"
                      "#define f16vec2  vec2 \n"
                      "#define f32vec2  vec2 \n"
                      "#define f64vec2  dvec2 \n\n"
                      "#define i8vec3   ivec3 \n"
                      "#define i16vec3  ivec3 \n"
                      "#define i32vec3  ivec3 \n"
                      "#define i64vec3  ivec3 \n"
                      "#define u8vec3   uvec3 \n"
                      "#define u16vec3  uvec3 \n"
                      "#define u32vec3  uvec3 \n"
                      "#define u64vec3  uvec3 \n"
                      "#define f16vec3  vec3 \n"
                      "#define f32vec3  vec3 \n"
                      "#define f64vec3  dvec3  \n\n"
                      "#define i8vec4   ivec4 \n"
                      "#define i16vec4  ivec4 \n"
                      "#define i32vec4  ivec4 \n"
                      "#define i64vec4  ivec4 \n"
                      "#define u8vec4   uvec4 \n"
                      "#define u16vec4  uvec4 \n"
                      "#define u32vec4  uvec4 \n"
                      "#define u64vec4  uvec4 \n"
                      "#define f16vec4  vec4 \n"
                      "#define f32vec4  vec4 \n"
                      "#define f64vec4  dvec4 \n";
            any_defines = true;
        }
        if (any_defines) { result += "\n"; }
    }

    result += "// ========================================================\n";

    return result;
}

} // namespace detail

const std::string& glsl_header() {
    static const std::string header = detail::build_glsl_header();
    return header;
}

/// Injects an arbitrary string into a given GLSL source code.
/// @return Modified source.
/// @throws OpenGLError   If the injection point could not be found.
std::string inject_header(const std::string& source, const std::string& injection) {
    if (injection.empty()) { return source; }

    const size_t injection_index = find_injection_index(source);
    if (injection_index == std::string::npos) {
        NOTF_THROW(OpenGLError, "Could not find injection point in given GLSL code");
    }

    return source.substr(0, injection_index) + injection + source.substr(injection_index, std::string::npos);
}

#ifdef NOTF_DEBUG
void assert_is_valid(const Shader& shader) {
    if (!shader.is_valid()) {
        NOTF_THROW(ResourceError, "Shader \"{}\" was deallocated! Has TheGraphicsSystem been deleted?",
                   shader.get_name());
    }
}
#else
void assert_is_valid(const Shader&) {} // noop
#endif

} // namespace

// shader =========================================================================================================== //

NOTF_OPEN_NAMESPACE

const Shader::Defines Shader::s_no_defines = {};

Shader::Shader(const GLuint id, Stage::Flags stages, std::string name)
    : m_name(std::move(name)), m_id(id), m_stages(stages) {
    // discover uniforms
    GLint uniform_count = 0;
    NOTF_CHECK_GL(glGetProgramiv(m_id.value(), GL_ACTIVE_UNIFORMS, &uniform_count));
    NOTF_ASSERT(uniform_count >= 0);
    m_uniforms.reserve(static_cast<size_t>(uniform_count));

    std::vector<GLchar> uniform_name(longest_uniform_length(m_id.value()));
    for (GLuint index = 0; index < static_cast<GLuint>(uniform_count); ++index) {
        Uniform variable;
        variable.type = 0;
        variable.size = 0;

        GLsizei name_length = 0;
        NOTF_CHECK_GL(glGetActiveUniform(m_id.value(), index, static_cast<GLsizei>(uniform_name.size()), &name_length,
                                         &variable.size, &variable.type, &uniform_name[0]));
        NOTF_ASSERT(variable.type);
        NOTF_ASSERT(variable.size);

        NOTF_ASSERT(name_length > 0);
        variable.name = std::string(&uniform_name[0], static_cast<size_t>(name_length));

        variable.location = glGetUniformLocation(m_id.value(), variable.name.c_str());
        if (variable.location == -1) {
            NOTF_LOG_WARN("Skipping uniform \"\" on Shader: \"{}\"", variable.name, m_name);
            // TODO: this is skipping uniforms in a named uniform block
            continue;
        }

        NOTF_LOG_TRACE("Discovered uniform \"{}\" on Shader: \"{}\"", variable.name, m_name);
        m_uniforms.emplace_back(std::move(variable));
    }
    m_uniforms.shrink_to_fit();
}

Shader::~Shader() { _deallocate(); }

#ifdef NOTF_DEBUG
bool Shader::validate_now() const {
    assert_is_valid(*this);

    GLint status = GL_FALSE;
    GLint message_size;
    NOTF_CHECK_GL(glValidateProgram(m_id.value()));
    NOTF_CHECK_GL(glGetProgramiv(m_id.value(), GL_VALIDATE_STATUS, &status));
    NOTF_CHECK_GL(glGetProgramiv(m_id.value(), GL_INFO_LOG_LENGTH, &message_size));
    std::vector<char> message = std::vector<char>(static_cast<size_t>(message_size));
    NOTF_CHECK_GL(glGetProgramInfoLog(m_id.value(), message_size, /*length*/ nullptr, &message[0]));

    NOTF_LOG_TRACE("Validation of Shader \"{}\" {}", m_name,
                   (status ? "succeeded" : fmt::format("failed:\n{}", message.data())));
    return status == GL_TRUE;
}
#endif

GLuint Shader::_build(const std::string& name, const Args& args) {
    if constexpr (config::is_debug_build()) { clear_gl_errors(); }

    // create the program
    // We don't use `glCreateShaderProgramv` so we could pass additional pre-link parameters.
    // For details, see:
    //     https://www.khronos.org/opengl/wiki/Interface_Matching#Separate_programs
    GLuint program = glCreateProgram();
    if (!program) { NOTF_THROW(OpenGLError, "Failed to create program object for Shader \"{}\"", name); }
    NOTF_CHECK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE));

    { // create and attach the shader stages
        GLuint vertex_stage = compile_stage(name, Shader::Stage::VERTEX, args.vertex_source);
        GLuint tess_ctrl_stage = compile_stage(name, Shader::Stage::TESS_CONTROL, args.tess_ctrl_source);
        GLuint tess_eval_stage = compile_stage(name, Shader::Stage::TESS_EVALUATION, args.tess_eval_source);
        GLuint geometry_stage = compile_stage(name, Shader::Stage::GEOMETRY, args.geometry_source);
        GLuint fragment_stage = compile_stage(name, Shader::Stage::FRAGMENT, args.fragment_source);
        GLuint compute_stage = compile_stage(name, Shader::Stage::COMPUTE, args.compute_source);

        if (vertex_stage) { NOTF_CHECK_GL(glAttachShader(program, vertex_stage)); }
        if (tess_ctrl_stage) { NOTF_CHECK_GL(glAttachShader(program, tess_ctrl_stage)); }
        if (tess_eval_stage) { NOTF_CHECK_GL(glAttachShader(program, tess_eval_stage)); }
        if (geometry_stage) { NOTF_CHECK_GL(glAttachShader(program, geometry_stage)); }
        if (fragment_stage) { NOTF_CHECK_GL(glAttachShader(program, fragment_stage)); }
        if (compute_stage) { NOTF_CHECK_GL(glAttachShader(program, compute_stage)); }

        NOTF_CHECK_GL(glLinkProgram(program));

        if (vertex_stage) {
            NOTF_CHECK_GL(glDetachShader(program, vertex_stage));
            NOTF_CHECK_GL(glDeleteShader(vertex_stage));
        }
        if (tess_ctrl_stage) {
            NOTF_CHECK_GL(glDetachShader(program, tess_ctrl_stage));
            NOTF_CHECK_GL(glDeleteShader(tess_ctrl_stage));
        }
        if (tess_eval_stage) {
            NOTF_CHECK_GL(glDetachShader(program, tess_eval_stage));
            NOTF_CHECK_GL(glDeleteShader(tess_eval_stage));
        }
        if (geometry_stage) {
            NOTF_CHECK_GL(glDetachShader(program, geometry_stage));
            NOTF_CHECK_GL(glDeleteShader(geometry_stage));
        }
        if (fragment_stage) {
            NOTF_CHECK_GL(glDetachShader(program, fragment_stage));
            NOTF_CHECK_GL(glDeleteShader(fragment_stage));
        }
        if (compute_stage) {
            NOTF_CHECK_GL(glDetachShader(program, compute_stage));
            NOTF_CHECK_GL(glDeleteShader(compute_stage));
        }
    }

    { // check for errors
        GLint success = GL_FALSE;
        NOTF_CHECK_GL(glGetProgramiv(program, GL_LINK_STATUS, &success));
        if (!success) {
            GLint error_size;
            NOTF_CHECK_GL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_size));
            std::vector<char> error_message(static_cast<size_t>(error_size));
            NOTF_CHECK_GL(glGetProgramInfoLog(program, error_size, /*length*/ nullptr, &error_message[0]));
            NOTF_CHECK_GL(glDeleteProgram(program));

            NOTF_THROW(OpenGLError, "Failed to link Shader program \"{}\":\n", name, error_message.data());
        }
    }
    NOTF_LOG_TRACE("Compiled and linked Shader program \"{}\".", name);

    NOTF_ASSERT(program);
    return program;
}

void Shader::_register_with_system(const ShaderPtr& shader) {
    NOTF_ASSERT(shader && shader->is_valid());
    TheGraphicsSystem::AccessFor<Shader>::register_new(shader);
}

const Shader::Uniform& Shader::_uniform(const std::string& name) const {
    assert_is_valid(*this);

    auto it = std::find_if(std::begin(m_uniforms), std::end(m_uniforms),
                           [&](const Uniform& var) -> bool { return var.name == name; });
    if (it == std::end(m_uniforms)) {
        NOTF_THROW(OpenGLError, "No uniform named \"{}\" in Shader \"{}\"", name, m_name);
    }
    return *it;
}

void Shader::_deallocate() {
    if (m_id.value()) {
        NOTF_CHECK_GL(glDeleteProgram(m_id.value()));
        NOTF_LOG_TRACE("Deleted Shader \"{}\"", m_name);
        m_id = ShaderId::invalid();
    }
}

template<>
void Shader::set_uniform(const std::string& name, const int& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_INT || uniform.type == GL_SAMPLER_2D) {
        NOTF_CHECK_GL(glProgramUniform1i(m_id.value(), uniform.location, value));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"int\"", name,
                   m_name, gl_type_name(uniform.type));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const unsigned int& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_UNSIGNED_INT) {
        NOTF_CHECK_GL(glProgramUniform1ui(m_id.value(), uniform.location, value));
    } else if (uniform.type == GL_SAMPLER_2D) {
        NOTF_CHECK_GL(glProgramUniform1i(m_id.value(), uniform.location, static_cast<GLint>(value)));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"unsigned int\"",
                   name, m_name, gl_type_name(uniform.type));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const float& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT) {
        NOTF_CHECK_GL(glProgramUniform1f(m_id.value(), uniform.location, value));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"float\"", name,
                   m_name, gl_type_name(uniform.type));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const V2f& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_VEC2) {
        NOTF_CHECK_GL(glProgramUniform2fv(m_id.value(), uniform.location, /*count*/ 1, value.as_ptr()));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"V2f\"", name,
                   m_name, gl_type_name(uniform.type));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const V4f& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_VEC4) {
        NOTF_CHECK_GL(glProgramUniform4fv(m_id.value(), uniform.location, /*count*/ 1, value.as_ptr()));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"V4f\"", name,
                   m_name, gl_type_name(uniform.type));
    }
}

template<>
void Shader::set_uniform(const std::string& name, const M4f& value) {
    assert_is_valid(*this);
    const Uniform& uniform = _uniform(name);
    if (uniform.type == GL_FLOAT_MAT4) {
        NOTF_CHECK_GL(glProgramUniformMatrix4fv(m_id.value(), uniform.location, /*count*/ 1, /*transpose*/ GL_FALSE,
                                                value.as_ptr()));
    } else {
        NOTF_THROW(OpenGLError,
                   "Uniform \"{}\" in Shader \"{}\" of type \"{}\" is not compatible with value type \"M4f\"", name,
                   m_name, gl_type_name(uniform.type));
    }
}

// vertex shader ==================================================================================================== //

VertexShaderPtr VertexShader::create(std::string name, const std::string& string, const Defines& defines) {
    std::string source = inject_header(string, glsl_header() + build_defines(defines));

    Args args;
    args.vertex_source = source.c_str();

    VertexShaderPtr shader = _create_shared(Shader::_build(name, args), name, std::move(source));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<VertexShader>().set(std::move(name), shader);
    return shader;
}

// tesselation shader =============================================================================================== //

TesselationShaderPtr TesselationShader::create(const std::string& name, const std::string& control_string,
                                               const std::string& evaluation_string, const Defines& defines) {
    const std::string injection_string = glsl_header() + build_defines(defines);
    const std::string modified_control_source = inject_header(control_string, injection_string);
    const std::string modified_evaluation_source = inject_header(evaluation_string, injection_string);

    Args args;
    args.tess_ctrl_source = modified_control_source.c_str();
    args.tess_eval_source = modified_evaluation_source.c_str();

    TesselationShaderPtr shader = _create_shared(Shader::_build(name, args), name, std::move(modified_control_source),
                                                 std::move(modified_evaluation_source));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<TesselationShader>().set(std::move(name), shader);
    return shader;
}

// geometry shader ================================================================================================== //

GeometryShaderPtr GeometryShader::create(std::string name, const std::string& string, const Defines& defines) {
    std::string source = inject_header(string, glsl_header() + build_defines(defines));

    Args args;
    args.geometry_source = source.c_str();

    GeometryShaderPtr shader = _create_shared(Shader::_build(name, args), name, std::move(source));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<GeometryShader>().set(std::move(name), shader);
    return shader;
}

// fragment shader ================================================================================================== //

FragmentShaderPtr FragmentShader::create(std::string name, const std::string& string, const Defines& defines) {
    std::string source = inject_header(string, glsl_header() + build_defines(defines));

    Args args;
    args.fragment_source = source.c_str();

    FragmentShaderPtr shader = _create_shared(Shader::_build(name, args), name, std::move(source));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<FragmentShader>().set(std::move(name), shader);
    return shader;
}

NOTF_CLOSE_NAMESPACE
