#include "graphics/core/gl_utils.hpp"

#include "common/half.hpp"
#include "common/log.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/opengl.hpp"

namespace notf {

bool gl_is_initialized() { return glGetString(GL_VERSION) != nullptr; }

void gl_log_system_info()
{
    log_info << "OpenGL ES version:    " << glGetString(GL_VERSION);
    log_info << "OpenGL ES vendor:     " << glGetString(GL_VENDOR);
    log_info << "OpenGL ES renderer:   " << glGetString(GL_RENDERER);
    log_info << "OpenGL ES extensions: " << glGetString(GL_EXTENSIONS);
    log_info << "GLSL version:         " << glGetString(GL_SHADING_LANGUAGE_VERSION);
}

const std::string& gl_type_name(GLenum type)
{
    static const std::string t_float                = "float";
    static const std::string t_vec2                 = "vec2";
    static const std::string t_vec3                 = "vec3";
    static const std::string t_vec4                 = "vec4";
    static const std::string t_int                  = "int";
    static const std::string t_ivec2                = "ivec2";
    static const std::string t_ivec3                = "ivec3";
    static const std::string t_ivec4                = "ivec4";
    static const std::string t_unsigned_int         = "unsigned int";
    static const std::string t_uvec2                = "uvec2";
    static const std::string t_uvec3                = "uvec3";
    static const std::string t_uvec4                = "uvec4";
    static const std::string t_bool                 = "bool";
    static const std::string t_bvec2                = "bvec2";
    static const std::string t_bvec3                = "bvec3";
    static const std::string t_bvec4                = "bvec4";
    static const std::string t_mat2                 = "mat2";
    static const std::string t_mat3                 = "mat3";
    static const std::string t_mat4                 = "mat4";
    static const std::string t_mat2x3               = "mat2x3";
    static const std::string t_mat2x4               = "mat2x4";
    static const std::string t_mat3x2               = "mat3x2";
    static const std::string t_mat3x4               = "mat3x4";
    static const std::string t_mat4x2               = "mat4x2";
    static const std::string t_mat4x3               = "mat4x3";
    static const std::string t_sampler2D            = "sampler2D";
    static const std::string t_sampler3D            = "sampler3D";
    static const std::string t_samplerCube          = "samplerCube";
    static const std::string t_sampler2DShadow      = "sampler2DShadow";
    static const std::string t_sampler2DArray       = "sampler2DArray";
    static const std::string t_sampler2DArrayShadow = "sampler2DArrayShadow";
    static const std::string t_sampler2DMS          = "sampler2DMS";
    static const std::string t_samplerCubeShadow    = "samplerCubeShadow";
    static const std::string t_isampler2D           = "isampler2D";
    static const std::string t_isampler3D           = "isampler3D";
    static const std::string t_isamplerCube         = "isamplerCube";
    static const std::string t_isampler2DArray      = "isampler2DArray";
    static const std::string t_isampler2DMS         = "isampler2DMS";
    static const std::string t_usampler2D           = "usampler2D";
    static const std::string t_usampler3D           = "usampler3D";
    static const std::string t_usamplerCube         = "usamplerCube";
    static const std::string t_usampler2DArray      = "usampler2DArray";
    static const std::string t_usampler2DMS         = "usampler2DMS";
    static const std::string t_image2D              = "image2D";
    static const std::string t_image3D              = "image3D";
    static const std::string t_imageCube            = "imageCube";
    static const std::string t_image2DArray         = "image2DArray";
    static const std::string t_iimage2D             = "iimage2D";
    static const std::string t_iimage3D             = "iimage3D";
    static const std::string t_iimageCube           = "iimageCube";
    static const std::string t_iimage2DArray        = "iimage2DArray";
    static const std::string t_uimage2D             = "uimage2D";
    static const std::string t_uimage3D             = "uimage3D";
    static const std::string t_uimageCube           = "uimageCube";
    static const std::string t_uimage2DArray        = "uimage2DArray";
    static const std::string t_atomic_uint          = "atomic_uint";
    static const std::string t_unknown              = "unknown";

    switch (type) {
    case GL_FLOAT:
        return t_float;
    case GL_FLOAT_VEC2:
        return t_vec2;
    case GL_FLOAT_VEC3:
        return t_vec3;
    case GL_FLOAT_VEC4:
        return t_vec4;
    case GL_INT:
        return t_int;
    case GL_INT_VEC2:
        return t_ivec2;
    case GL_INT_VEC3:
        return t_ivec3;
    case GL_INT_VEC4:
        return t_ivec4;
    case GL_UNSIGNED_INT:
        return t_unsigned_int;
    case GL_UNSIGNED_INT_VEC2:
        return t_uvec2;
    case GL_UNSIGNED_INT_VEC3:
        return t_uvec3;
    case GL_UNSIGNED_INT_VEC4:
        return t_uvec4;
    case GL_BOOL:
        return t_bool;
    case GL_BOOL_VEC2:
        return t_bvec2;
    case GL_BOOL_VEC3:
        return t_bvec3;
    case GL_BOOL_VEC4:
        return t_bvec4;
    case GL_FLOAT_MAT2:
        return t_mat2;
    case GL_FLOAT_MAT3:
        return t_mat3;
    case GL_FLOAT_MAT4:
        return t_mat4;
    case GL_FLOAT_MAT2x3:
        return t_mat2x3;
    case GL_FLOAT_MAT2x4:
        return t_mat2x4;
    case GL_FLOAT_MAT3x2:
        return t_mat3x2;
    case GL_FLOAT_MAT3x4:
        return t_mat3x4;
    case GL_FLOAT_MAT4x2:
        return t_mat4x2;
    case GL_FLOAT_MAT4x3:
        return t_mat4x3;
    case GL_SAMPLER_2D:
        return t_sampler2D;
    case GL_SAMPLER_3D:
        return t_sampler3D;
    case GL_SAMPLER_CUBE:
        return t_samplerCube;
    case GL_SAMPLER_2D_SHADOW:
        return t_sampler2DShadow;
    case GL_SAMPLER_2D_ARRAY:
        return t_sampler2DArray;
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        return t_sampler2DArrayShadow;
    case GL_SAMPLER_2D_MULTISAMPLE:
        return t_sampler2DMS;
    case GL_SAMPLER_CUBE_SHADOW:
        return t_samplerCubeShadow;
    case GL_INT_SAMPLER_2D:
        return t_isampler2D;
    case GL_INT_SAMPLER_3D:
        return t_isampler3D;
    case GL_INT_SAMPLER_CUBE:
        return t_isamplerCube;
    case GL_INT_SAMPLER_2D_ARRAY:
        return t_isampler2DArray;
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
        return t_isampler2DMS;
    case GL_UNSIGNED_INT_SAMPLER_2D:
        return t_usampler2D;
    case GL_UNSIGNED_INT_SAMPLER_3D:
        return t_usampler3D;
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return t_usamplerCube;
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        return t_usampler2DArray;
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        return t_usampler2DMS;
    case GL_IMAGE_2D:
        return t_image2D;
    case GL_IMAGE_3D:
        return t_image3D;
    case GL_IMAGE_CUBE:
        return t_imageCube;
    case GL_IMAGE_2D_ARRAY:
        return t_image2DArray;
    case GL_INT_IMAGE_2D:
        return t_iimage2D;
    case GL_INT_IMAGE_3D:
        return t_iimage3D;
    case GL_INT_IMAGE_CUBE:
        return t_iimageCube;
    case GL_INT_IMAGE_2D_ARRAY:
        return t_iimage2DArray;
    case GL_UNSIGNED_INT_IMAGE_2D:
        return t_uimage2D;
    case GL_UNSIGNED_INT_IMAGE_3D:
        return t_uimage3D;
    case GL_UNSIGNED_INT_IMAGE_CUBE:
        return t_uimageCube;
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
        return t_uimage2DArray;
    case GL_UNSIGNED_INT_ATOMIC_COUNTER:
        return t_atomic_uint;
    }
    return t_unknown;
}

//====================================================================================================================//

GLenum to_gl_type(const GLbyte&) { return GL_BYTE; }
GLenum to_gl_type(const GLubyte&) { return GL_UNSIGNED_BYTE; }
GLenum to_gl_type(const GLshort&) { return GL_SHORT; }
GLenum to_gl_type(const GLushort&) { return GL_UNSIGNED_SHORT; }
GLenum to_gl_type(const GLint&) { return GL_INT; }
GLenum to_gl_type(const GLuint&) { return GL_UNSIGNED_INT; }
GLenum to_gl_type(const half&) { return GL_HALF_FLOAT; }
GLenum to_gl_type(const GLfloat&) { return GL_FLOAT; }

//====================================================================================================================//

VaoBindGuard::VaoBindGuard(GLuint vao) : m_vao(vao) { gl_check(glBindVertexArray(m_vao)); }

VaoBindGuard::~VaoBindGuard() { gl_check(glBindVertexArray(0)); }

} // namespace notf
