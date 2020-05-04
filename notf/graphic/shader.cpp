#include "notf/graphic/shader.hpp"

#include <regex>
#include <sstream>

#include "notf/meta/log.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/graphics_system.hpp"

NOTF_USING_NAMESPACE;

namespace { // anonymous

/// Compiles a single Shader stage from a given source.
/// @param program_name Name of the Shader program (for error messages).
/// @param stage        Shader stage produced by the source.
/// @param source       Source to compile.
/// @return OpenGL ID of the Shader stage.
GLuint compile_stage(const std::string& program_name, const AnyShader::Stage::Flag stage, const char* source) {
    if (!source) { return 0; }

    const char* stage_name = AnyShader::Stage::get_name(stage);

    // create the OpenGL Shader
    GLuint shader = 0;
    switch (stage) {
    case AnyShader::Stage::VERTEX: shader = glCreateShader(GL_VERTEX_SHADER); break;
    case AnyShader::Stage::TESS_CONTROL: shader = glCreateShader(GL_TESS_CONTROL_SHADER); break;
    case AnyShader::Stage::TESS_EVALUATION: shader = glCreateShader(GL_TESS_EVALUATION_SHADER); break;
    case AnyShader::Stage::GEOMETRY: shader = glCreateShader(GL_GEOMETRY_SHADER); break;
    case AnyShader::Stage::FRAGMENT: shader = glCreateShader(GL_FRAGMENT_SHADER); break;
    case AnyShader::Stage::COMPUTE: shader = glCreateShader(GL_COMPUTE_SHADER); break;
    }

    if (!shader) {
        NOTF_THROW(OpenGLError, "Failed to create OpenGL {} shader object for for Shader \"{}\"", stage_name,
                   program_name);
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
        NOTF_THROW(OpenGLError, "Failed to compile {} stage for Shader \"{}\"\n{}", stage_name, program_name,
                   error_message.data());
    }

    NOTF_ASSERT(shader);
    return shader;
}

/// Finds the index in a given GLSL source string where custom `#define`s can be injected.
size_t find_injection_index(const std::string& source) {
    static const std::regex version_regex(R"==(\n?\s*#version\s*\d{3}\s*es[ \t]*\n)==");
    static const std::regex extensions_regex(R"==(\n\s*#extension\s*\w*\s*:\s*(?:require|enable|warn|disable)\s*\n)==");

    size_t injection_index = std::string::npos;

    std::smatch extensions;
    std::regex_search(source, extensions, extensions_regex);
    if (!extensions.empty()) {
        // if the Shader contains one or more #extension strings, we have to inject the `#define`s after those
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
std::string parse_definitions(const AnyShader::Definitions& definitions) {
    std::stringstream ss;
    for (const AnyShader::Definition& definition : definitions) {
        ss << "#define " << definition.name << " " << definition.value << "\n";
    }
    return ss.str();
}

std::string _build_glsl_header() {
    std::string result = "\n//==== notf header ========================================\n\n";

    const auto& extensions = TheGraphicsSystem::get_extensions();

    { // extensions first ...
        bool any_extensions = false;
        if (extensions.gpu_shader5) {
            result += "#extension GL_EXT_gpu_shader5 : enable\n";
            any_extensions = true;
        }
        if (any_extensions) { result += "\n"; }
    }

    { // ... then pragmas ...
        bool any_pragma = false;
#ifdef NOTF_DEBUG
        any_pragma = true;
        result += "#pragma debug(on)\n";
#endif
        if (any_pragma) { result += "\n"; }
    }

    { // ... and definitions last
        bool any_definitions = false;
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
            any_definitions = true;
        }
        if (any_definitions) { result += "\n"; }
    }

    result += "// ========================================================\n";

    return result;
}

const std::string& glsl_header() {
    static const std::string header = _build_glsl_header();
    return header;
}

/// Injects an arbitrary string into a given GLSL source code.
/// @return Modified source.
/// @throws OpenGLError   If the injection point could not be found.
std::string glsl_injection(const std::string& source, const std::string& injection) {
    if (injection.empty()) { return source; }

    const size_t injection_index = find_injection_index(source);
    if (injection_index == std::string::npos) {
        NOTF_THROW(OpenGLError, "Could not find injection point in given GLSL code");
    }

    return source.substr(0, injection_index) + injection + source.substr(injection_index, std::string::npos);
}

void assert_is_valid(const AnyShader& shader) {
    if constexpr (config::is_debug_build()) {
        if (!shader.is_valid()) {
            NOTF_THROW(ResourceError, "Shader \"{}\" was deallocated! Has TheGraphicsSystem been deleted?",
                       shader.get_name());
        }
    } else {
        // noop
    }
}

} // namespace

// shader =========================================================================================================== //

AnyShader::~AnyShader() { _deallocate(); }

std::string AnyShader::inject_header(const std::string& source, const Definitions& definitions) {
    if (source.empty()) { return {}; }
    return glsl_injection(source, glsl_header() + parse_definitions(definitions));
}

#ifdef NOTF_DEBUG
bool AnyShader::validate_now() const {
    assert_is_valid(*this);

    GLint status = GL_FALSE;
    GLint message_size;
    NOTF_CHECK_GL(glValidateProgram(m_id.get_value()));
    NOTF_CHECK_GL(glGetProgramiv(m_id.get_value(), GL_VALIDATE_STATUS, &status));
    NOTF_CHECK_GL(glGetProgramiv(m_id.get_value(), GL_INFO_LOG_LENGTH, &message_size));
    std::vector<char> message = std::vector<char>(static_cast<size_t>(message_size));
    NOTF_CHECK_GL(glGetProgramInfoLog(m_id.get_value(), message_size, /*length*/ nullptr, &message[0]));

    NOTF_LOG_TRACE("Validation of Shader \"{}\" {}", m_name,
                   (status ? "succeeded" : fmt::format("failed:\n{}", message.data())));
    return status == GL_TRUE;
}
#endif

GLuint AnyShader::_build(const std::string& name, const Args& args) {
    if constexpr (config::is_debug_build()) { clear_gl_errors(); }

    // create the program
    // We don't use `glCreateShaderProgramv` so we can pass additional pre-link parameters.
    // For details, see:
    //     https://www.khronos.org/opengl/wiki/Interface_Matching#Separate_programs
    GLuint program = glCreateProgram();
    if (!program) { NOTF_THROW(OpenGLError, "Failed to create program object for Shader \"{}\"", name); }
    NOTF_CHECK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE));

    { // create and attach the shader stages
        GLuint vertex_stage = compile_stage(name, AnyShader::Stage::VERTEX, args.vertex_source);
        GLuint tess_ctrl_stage = compile_stage(name, AnyShader::Stage::TESS_CONTROL, args.tess_ctrl_source);
        GLuint tess_eval_stage = compile_stage(name, AnyShader::Stage::TESS_EVALUATION, args.tess_eval_source);
        GLuint geometry_stage = compile_stage(name, AnyShader::Stage::GEOMETRY, args.geometry_source);
        GLuint fragment_stage = compile_stage(name, AnyShader::Stage::FRAGMENT, args.fragment_source);
        GLuint compute_stage = compile_stage(name, AnyShader::Stage::COMPUTE, args.compute_source);

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

            NOTF_THROW(OpenGLError, "Failed to link Shader \"{}\"\n{}", name, error_message.data());
        }
    }
    NOTF_LOG_TRACE("Compiled and linked Shader \"{}\".", name);

    NOTF_ASSERT(program);
    return program;
}

void AnyShader::_register_with_system(const AnyShaderPtr& shader) {
    NOTF_ASSERT(shader && shader->is_valid());
    TheGraphicsSystem::AccessFor<AnyShader>::register_new(shader);
}

void AnyShader::_deallocate() {
    if (!m_id.get_value()) { return; }

    NOTF_CHECK_GL(glDeleteProgram(m_id.get_value()));
    m_id = ShaderId::invalid();

    NOTF_LOG_TRACE("Deleted Shader \"{}\"", m_name);
}

// vertex shader ==================================================================================================== //

VertexShader::~VertexShader() = default;

VertexShaderPtr VertexShader::create(std::string name, std::string source, const Definitions& definitions) {
    std::string modified_source = inject_header(source, definitions);

    Args args;
    args.vertex_source = modified_source.c_str();

    VertexShaderPtr shader
        = _create_shared(AnyShader::_build(name, args), name, std::move(source), std::move(definitions));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<VertexShader>().set(std::move(name), shader);
    return shader;
}

// tesselation shader =============================================================================================== //

TesselationShader::~TesselationShader() = default;

TesselationShaderPtr TesselationShader::create(std::string name, std::string control_source,
                                               std::string evaluation_source, const Definitions& definitions) {
    const std::string header = glsl_header() + parse_definitions(definitions);
    const std::string modified_control_source = glsl_injection(control_source, header);
    const std::string modified_evaluation_source = glsl_injection(evaluation_source, header);

    Args args;
    args.tess_ctrl_source = modified_control_source.c_str();
    args.tess_eval_source = modified_evaluation_source.c_str();

    TesselationShaderPtr shader = _create_shared(AnyShader::_build(name, args), name, std::move(control_source),
                                                 std::move(evaluation_source), std::move(definitions));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<TesselationShader>().set(std::move(name), shader);
    return shader;
}

// geometry shader ================================================================================================== //

GeometryShader::~GeometryShader() = default;

GeometryShaderPtr GeometryShader::create(std::string name, std::string source, const Definitions& definitions) {
    std::string modified_source = inject_header(source, definitions);

    Args args;
    args.geometry_source = modified_source.c_str();

    GeometryShaderPtr shader
        = _create_shared(AnyShader::_build(name, args), name, std::move(source), std::move(definitions));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<GeometryShader>().set(std::move(name), shader);
    return shader;
}

// fragment shader ================================================================================================== //

FragmentShader::~FragmentShader() = default;

FragmentShaderPtr FragmentShader::create(std::string name, std::string source, const Definitions& definitions) {
    std::string modified_source = inject_header(source, definitions);

    Args args;
    args.fragment_source = modified_source.c_str();

    FragmentShaderPtr shader
        = _create_shared(AnyShader::_build(name, args), name, std::move(source), std::move(definitions));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<FragmentShader>().set(std::move(name), shader);
    return shader;
}

// multi stage shader =============================================================================================== //

MultiStageShader::~MultiStageShader() = default;

MultiStageShaderPtr MultiStageShader::create(std::string name, Sources sources, const Definitions& definitions) {

    Stage::Flags stages = 0;
    const std::string header = glsl_header() + parse_definitions(definitions);
    Args args;
    if (!sources.vertex.empty()) {
        stages |= Stage::VERTEX;
        sources.vertex = glsl_injection(sources.vertex, header);
        args.vertex_source = sources.vertex.c_str();
    }
    if (!sources.tesselation_control.empty()) {
        stages |= Stage::TESS_CONTROL;
        sources.tesselation_control = glsl_injection(sources.tesselation_control, header);
        args.tess_ctrl_source = sources.tesselation_control.c_str();
    }
    if (!sources.tesselation_evaluation.empty()) {
        stages |= Stage::TESS_EVALUATION;
        sources.tesselation_evaluation = glsl_injection(sources.tesselation_evaluation, header);
        args.tess_eval_source = sources.tesselation_evaluation.c_str();
    }
    if (!sources.geometry.empty()) {
        stages |= Stage::GEOMETRY;
        sources.geometry = glsl_injection(sources.geometry, header);
        args.geometry_source = sources.geometry.c_str();
    }
    if (!sources.fragment.empty()) {
        stages |= Stage::FRAGMENT;
        sources.fragment = glsl_injection(sources.fragment, header);
        args.fragment_source = sources.fragment.c_str();
    }
    if (!sources.compute.empty()) {
        stages |= Stage::COMPUTE;
        sources.compute = glsl_injection(sources.compute, header);
        args.compute_source = sources.compute.c_str();
    }

    MultiStageShaderPtr shader
        = _create_shared(AnyShader::_build(name, args), name, std::move(sources), stages, std::move(definitions));
    _register_with_system(shader);
    ResourceManager::get_instance().get_type<MultiStageShader>().set(std::move(name), shader);
    return shader;
}
