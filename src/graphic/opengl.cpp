#include "notf/graphic/opengl.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/glfw.hpp"

NOTF_USING_NAMESPACE;

namespace { // anonymous

/// String representation of an OpenGL error code.
/// Is a constant expression so it can be used as optimizable input to log_* - functions.
/// @param error_code    The OpenGL error code.
/// @return              The given error code as a string, empty if GL_NO_ERROR was passed.
constexpr const char* gl_error_string(GLenum error_code) {
    const char* no_error = "GL_NO_ERROR";
    const char* invalid_enum = "GL_INVALID_ENUM";
    const char* invalid_value = "GL_INVALID_VALUE";
    const char* invalid_operation = "GL_INVALID_OPERATION";
    const char* out_of_memory = "GL_OUT_OF_MEMORY";
    const char* invalid_framebuffer_operation = "GL_INVALID_FRAMEBUFFER_OPERATION";
#ifdef GL_STACK_OVERFLOW
    const char* stack_overflow = "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
    const char* stack_underflow = "GL_STACK_UNDERFLOW";
#endif

    switch (error_code) {
    case GL_NO_ERROR: // No user error reported since last call to glGetError.
        return no_error;
    case GL_INVALID_ENUM: // Set when an enumeration parameter is not legal.
        return invalid_enum;
    case GL_INVALID_VALUE: // Set when a value parameter is not legal.
        return invalid_value;
    case GL_INVALID_OPERATION: // Set when the state for a command is not legal for its given parameters.
        return invalid_operation;
    case GL_OUT_OF_MEMORY: // Set when a memory allocation operation cannot allocate (enough) memory.
        return out_of_memory;
    case GL_INVALID_FRAMEBUFFER_OPERATION: // Set when reading or writing to a framebuffer that is not complete.
        return invalid_framebuffer_operation;
#ifdef GL_STACK_OVERFLOW
    case GL_STACK_OVERFLOW: // Set when a stack pushing operation causes a stack overflow.
        return stack_overflow;
#endif
#ifdef GL_STACK_UNDERFLOW
    case GL_STACK_UNDERFLOW: // Set when a stack popping operation occurs while the stack is at its lowest point.
        return stack_underflow;
#endif
    default: return "";
    }
}

std::tuple<GLenum, GLenum, GLenum, GLenum> convert_blend_mode(const BlendMode& mode) {
    static auto get_factors = [](const BlendMode::Mode blend_mode) -> std::pair<GLenum, GLenum> {
        GLenum source_factor = 0;
        GLenum destination_factor = 0;
        switch (blend_mode) {
        case (BlendMode::SOURCE_OVER):
            source_factor = GL_ONE;
            destination_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
        case (BlendMode::SOURCE_IN):
            source_factor = GL_DST_ALPHA;
            destination_factor = GL_ZERO;
            break;
        case (BlendMode::SOURCE_OUT):
            source_factor = GL_ONE_MINUS_DST_ALPHA;
            destination_factor = GL_ZERO;
            break;
        case (BlendMode::SOURCE_ATOP):
            source_factor = GL_DST_ALPHA;
            destination_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
        case (BlendMode::DESTINATION_OVER):
            source_factor = GL_ONE_MINUS_DST_ALPHA;
            destination_factor = GL_ONE;
            break;
        case (BlendMode::DESTINATION_IN):
            source_factor = GL_ZERO;
            destination_factor = GL_SRC_ALPHA;
            break;
        case (BlendMode::DESTINATION_OUT):
            source_factor = GL_ZERO;
            destination_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
        case (BlendMode::DESTINATION_ATOP):
            source_factor = GL_ONE_MINUS_DST_ALPHA;
            destination_factor = GL_SRC_ALPHA;
            break;
        case (BlendMode::LIGHTER):
            source_factor = GL_ONE;
            destination_factor = GL_ONE;
            break;
        case (BlendMode::COPY):
            source_factor = GL_ONE;
            destination_factor = GL_ZERO;
            break;
        case (BlendMode::XOR):
            source_factor = GL_ONE_MINUS_DST_ALPHA;
            destination_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
        }
        return {source_factor, destination_factor};
    };
    const auto rgb_factors = get_factors(mode.rgb);
    const auto alpha_factors = get_factors(mode.alpha);
    return std::make_tuple(rgb_factors.first, rgb_factors.second, alpha_factors.first, alpha_factors.second);
}

} // namespace

// blend mode ======================================================================================================= //

BlendMode::OpenGLBlendMode::OpenGLBlendMode(const BlendMode& mode) : OpenGLBlendMode(convert_blend_mode(mode)) {}

// data usage ======================================================================================================= //

GLenum get_gl_usage(const GLUsage usage) {
    switch (usage) {
    case GLUsage::DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
    case GLUsage::DYNAMIC_READ: return GL_DYNAMIC_READ;
    case GLUsage::DYNAMIC_COPY: return GL_DYNAMIC_COPY;
    case GLUsage::STATIC_DRAW: return GL_STATIC_DRAW;
    case GLUsage::STATIC_READ: return GL_STATIC_READ;
    case GLUsage::STATIC_COPY: return GL_STATIC_COPY;
    case GLUsage::STREAM_DRAW: return GL_STREAM_DRAW;
    case GLUsage::STREAM_READ: return GL_STREAM_READ;
    case GLUsage::STREAM_COPY: return GL_STREAM_COPY;
    }
}

// data types ======================================================================================================= //

const char* get_gl_type_name(GLenum type) {
    static const char* name_of_float = "float";
    static const char* name_of_vec2 = "vec2";
    static const char* name_of_vec3 = "vec3";
    static const char* name_of_vec4 = "vec4";
    static const char* name_of_int = "int";
    static const char* name_of_ivec2 = "ivec2";
    static const char* name_of_ivec3 = "ivec3";
    static const char* name_of_ivec4 = "ivec4";
    static const char* name_of_unsigned_int = "unsigned int";
    static const char* name_of_uvec2 = "uvec2";
    static const char* name_of_uvec3 = "uvec3";
    static const char* name_of_uvec4 = "uvec4";
    static const char* name_of_bool = "bool";
    static const char* name_of_bvec2 = "bvec2";
    static const char* name_of_bvec3 = "bvec3";
    static const char* name_of_bvec4 = "bvec4";
    static const char* name_of_mat2 = "mat2";
    static const char* name_of_mat3 = "mat3";
    static const char* name_of_mat4 = "mat4";
    static const char* name_of_mat2x3 = "mat2x3";
    static const char* name_of_mat2x4 = "mat2x4";
    static const char* name_of_mat3x2 = "mat3x2";
    static const char* name_of_mat3x4 = "mat3x4";
    static const char* name_of_mat4x2 = "mat4x2";
    static const char* name_of_mat4x3 = "mat4x3";
    static const char* name_of_sampler2D = "sampler2D";
    static const char* name_of_sampler3D = "sampler3D";
    static const char* name_of_samplerCube = "samplerCube";
    static const char* name_of_sampler2DShadow = "sampler2DShadow";
    static const char* name_of_sampler2DArray = "sampler2DArray";
    static const char* name_of_sampler2DArrayShadow = "sampler2DArrayShadow";
    static const char* name_of_sampler2DMS = "sampler2DMS";
    static const char* name_of_samplerCubeShadow = "samplerCubeShadow";
    static const char* name_of_isampler2D = "isampler2D";
    static const char* name_of_isampler3D = "isampler3D";
    static const char* name_of_isamplerCube = "isamplerCube";
    static const char* name_of_isampler2DArray = "isampler2DArray";
    static const char* name_of_isampler2DMS = "isampler2DMS";
    static const char* name_of_usampler2D = "usampler2D";
    static const char* name_of_usampler3D = "usampler3D";
    static const char* name_of_usamplerCube = "usamplerCube";
    static const char* name_of_usampler2DArray = "usampler2DArray";
    static const char* name_of_usampler2DMS = "usampler2DMS";
    static const char* name_of_image2D = "image2D";
    static const char* name_of_image3D = "image3D";
    static const char* name_of_imageCube = "imageCube";
    static const char* name_of_image2DArray = "image2DArray";
    static const char* name_of_iimage2D = "iimage2D";
    static const char* name_of_iimage3D = "iimage3D";
    static const char* name_of_iimageCube = "iimageCube";
    static const char* name_of_iimage2DArray = "iimage2DArray";
    static const char* name_of_uimage2D = "uimage2D";
    static const char* name_of_uimage3D = "uimage3D";
    static const char* name_of_uimageCube = "uimageCube";
    static const char* name_of_uimage2DArray = "uimage2DArray";
    static const char* name_of_atomic_uint = "atomic_uint";
    static const char* name_of_unknown = "unknown";

    switch (type) {
    case GL_FLOAT: return name_of_float;
    case GL_FLOAT_VEC2: return name_of_vec2;
    case GL_FLOAT_VEC3: return name_of_vec3;
    case GL_FLOAT_VEC4: return name_of_vec4;
    case GL_INT: return name_of_int;
    case GL_INT_VEC2: return name_of_ivec2;
    case GL_INT_VEC3: return name_of_ivec3;
    case GL_INT_VEC4: return name_of_ivec4;
    case GL_UNSIGNED_INT: return name_of_unsigned_int;
    case GL_UNSIGNED_INT_VEC2: return name_of_uvec2;
    case GL_UNSIGNED_INT_VEC3: return name_of_uvec3;
    case GL_UNSIGNED_INT_VEC4: return name_of_uvec4;
    case GL_BOOL: return name_of_bool;
    case GL_BOOL_VEC2: return name_of_bvec2;
    case GL_BOOL_VEC3: return name_of_bvec3;
    case GL_BOOL_VEC4: return name_of_bvec4;
    case GL_FLOAT_MAT2: return name_of_mat2;
    case GL_FLOAT_MAT3: return name_of_mat3;
    case GL_FLOAT_MAT4: return name_of_mat4;
    case GL_FLOAT_MAT2x3: return name_of_mat2x3;
    case GL_FLOAT_MAT2x4: return name_of_mat2x4;
    case GL_FLOAT_MAT3x2: return name_of_mat3x2;
    case GL_FLOAT_MAT3x4: return name_of_mat3x4;
    case GL_FLOAT_MAT4x2: return name_of_mat4x2;
    case GL_FLOAT_MAT4x3: return name_of_mat4x3;
    case GL_SAMPLER_2D: return name_of_sampler2D;
    case GL_SAMPLER_3D: return name_of_sampler3D;
    case GL_SAMPLER_CUBE: return name_of_samplerCube;
    case GL_SAMPLER_2D_SHADOW: return name_of_sampler2DShadow;
    case GL_SAMPLER_2D_ARRAY: return name_of_sampler2DArray;
    case GL_SAMPLER_2D_ARRAY_SHADOW: return name_of_sampler2DArrayShadow;
    case GL_SAMPLER_2D_MULTISAMPLE: return name_of_sampler2DMS;
    case GL_SAMPLER_CUBE_SHADOW: return name_of_samplerCubeShadow;
    case GL_INT_SAMPLER_2D: return name_of_isampler2D;
    case GL_INT_SAMPLER_3D: return name_of_isampler3D;
    case GL_INT_SAMPLER_CUBE: return name_of_isamplerCube;
    case GL_INT_SAMPLER_2D_ARRAY: return name_of_isampler2DArray;
    case GL_INT_SAMPLER_2D_MULTISAMPLE: return name_of_isampler2DMS;
    case GL_UNSIGNED_INT_SAMPLER_2D: return name_of_usampler2D;
    case GL_UNSIGNED_INT_SAMPLER_3D: return name_of_usampler3D;
    case GL_UNSIGNED_INT_SAMPLER_CUBE: return name_of_usamplerCube;
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return name_of_usampler2DArray;
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return name_of_usampler2DMS;
    case GL_IMAGE_2D: return name_of_image2D;
    case GL_IMAGE_3D: return name_of_image3D;
    case GL_IMAGE_CUBE: return name_of_imageCube;
    case GL_IMAGE_2D_ARRAY: return name_of_image2DArray;
    case GL_INT_IMAGE_2D: return name_of_iimage2D;
    case GL_INT_IMAGE_3D: return name_of_iimage3D;
    case GL_INT_IMAGE_CUBE: return name_of_iimageCube;
    case GL_INT_IMAGE_2D_ARRAY: return name_of_iimage2DArray;
    case GL_UNSIGNED_INT_IMAGE_2D: return name_of_uimage2D;
    case GL_UNSIGNED_INT_IMAGE_3D: return name_of_uimage3D;
    case GL_UNSIGNED_INT_IMAGE_CUBE: return name_of_uimageCube;
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY: return name_of_uimage2DArray;
    case GL_UNSIGNED_INT_ATOMIC_COUNTER: return name_of_atomic_uint;
    }
    return name_of_unknown;
}

// opengl error handling ============================================================================================ //

namespace detail {

void check_gl_error(uint line, const char* file) {
    if (glfwGetCurrentContext() == nullptr) { NOTF_THROW(OpenGLError, "No OpenGL context current on this thread"); }

    GLenum code;
    while ((code = glGetError()) != GL_NO_ERROR) {
        TheLogger::get()->error("OpenGL error: {} ({}:{})", gl_error_string(code), filename_from_path(file), line);
    }
}

} // namespace detail

void clear_gl_errors() {
    if constexpr (config::is_debug_build()) {
        while (glGetError() != GL_NO_ERROR) {};
    }
}
