/// Collection of various C++ types representing different OpenGL enumerations and OpenGL related utility functions.
#pragma once

#include "glad/glad.h"

#include "notf/meta/bits.hpp"
#include "notf/meta/exception.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// blend mode ======================================================================================================= //

/// HTML5 canvas-like approach to blending the results of multiple OpenGL drawings.
/// Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
/// The source image is the image being rendered, and the destination image the current state of the bitmap.
struct BlendMode {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Blend mode, can be set for RGB and the alpha channel separately.
    enum Mode : GLubyte {
        SOURCE_OVER,      ///< Display the source image wherever the source image is opaque, the destination image
                          /// elsewhere (most common mode).
        SOURCE_IN,        ///< Display the source image where the both are opaque, transparency elsewhere.
        SOURCE_OUT,       ///< Display the source image where the the source is opaque and the destination transparent,
                          ///  transparency elsewhere.
        SOURCE_ATOP,      ///< Source image wherever both images are opaque.
                          ///  Display the destination image wherever the destination image is opaque but the source
                          ///  image is transparent. Display transparency elsewhere.
        DESTINATION_OVER, ///< Same as SOURCE_OVER with the destination instead of the source.
        DESTINATION_IN,   ///< Same as SOURCE_IN with the destination instead of the source.
        DESTINATION_OUT,  ///< Same as SOURCE_OUT with the destination instead of the source.
        DESTINATION_ATOP, ///< Same as SOURCE_ATOP with the destination instead of the source.
        LIGHTER,          ///< The sum of the source image and destination image, with 255 (100%) as a limit.
        COPY,             ///< Source image instead of the destination image (overwrite destination).
        XOR,              ///< Exclusive OR of the source image and destination image.
        _DEFAULT = COPY,  ///< Provides the default values for `glBlendFuncSeparate`
    };

    /// Structure used to translate a notf::BlendMode into a pair of enums that can be used with OpenGL.
    struct OpenGLBlendMode {

        // methods --------------------------------------------------------- //
    private:
        /// Initializing constructor.
        OpenGLBlendMode(std::tuple<GLenum, GLenum, GLenum, GLenum> modes)
            : source_rgb(std::get<0>(modes))
            , destination_rgb(std::get<1>(modes))
            , source_alpha(std::get<2>(modes))
            , destination_alpha(std::get<3>(modes)) {}

    public:
        /// Constructor.
        OpenGLBlendMode(const BlendMode& mode);

        // fields ---------------------------------------------------------- //
    public:
        /// Source factor.
        const GLenum source_rgb;

        /// Destination factor.
        const GLenum destination_rgb;

        /// Source factor.
        const GLenum source_alpha;

        /// Destination factor.
        const GLenum destination_alpha;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    BlendMode() = default;

    /// Value constructor.
    /// @param mode Single blend mode for both rgb and alpha.
    BlendMode(const Mode mode) : rgb(mode), alpha(mode) {}

    /// Separate blend modes for both rgb and alpha.
    /// @param color    RGB blend mode.
    /// @param alpha    Alpha blend mode.
    BlendMode(const Mode color, const Mode alpha) : rgb(color), alpha(alpha) {}

    /// Equality operator.
    /// @param other    Blend mode to test against.
    bool operator==(const BlendMode& other) const { return rgb == other.rgb && alpha == other.alpha; }

    /// Inequality operator.
    /// @param other    Blend mode to test against.
    bool operator!=(const BlendMode& other) const { return rgb != other.rgb || alpha != other.alpha; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Blend mode for colors.
    Mode rgb = _DEFAULT;

    /// Blend mode for transparency.
    Mode alpha = _DEFAULT;
};

// cull face ======================================================================================================== //

/// Direction to cull in the culling test.
enum class CullFace : GLenum {
    BACK = GL_BACK,           ///< Do not render back-facing faces (default).
    FRONT = GL_FRONT,         ///< Do not render front-facing faces.
    BOTH = GL_FRONT_AND_BACK, ///< Cull all faces.
    NONE = GL_NONE,           ///< Render both front- and back-facing faces.
    DEFAULT = BACK,
};

struct GLBuffer {
    enum Flag : unsigned char {
        COLOR = 1u << 1,  //< Color buffer
        DEPTH = 1u << 2,  //< Depth buffer
        STENCIL = 1u << 3 //< Stencil buffer
    };
};
using GLBuffers = std::underlying_type_t<GLBuffer::Flag>;

// stencil mask ===================================================================================================== //

struct StencilMask {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    /// Returns a mask with all bits set to 1 (initial value of an OpenGL context).
    StencilMask() = default;

    /// Single value for both front- and back facing stencil mask.
    StencilMask(const GLuint mask) : front(mask), back(mask) {}

    /// Separate values for front- and back facing stencil mask.
    StencilMask(const GLuint front, const GLuint back) : front(front), back(back) {}

    /// Stencil with all bits set to one (same as default).
    static StencilMask all_one() { return StencilMask(); }

    /// Stencil with all bits set to zero.
    static StencilMask all_zero() { return StencilMask(0); }

    /// Front stencil mask set to all one, back to all zero.
    static StencilMask front_only() { return StencilMask(all_bits_one<GLuint>(), 0); }

    /// Front stencil mask set to all zero, back to all one.
    static StencilMask back_only() { return StencilMask(0, all_bits_one<GLuint>()); }

    /// Equality operator.
    /// @param other    StencilMask to test against.
    bool operator==(const StencilMask& other) const { return front == other.front && back == other.back; }

    /// Inequality operator.
    /// @param other    StencilMask to test against.
    bool operator!=(const StencilMask& other) const { return front != other.front || back != other.back; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Mask for front face stencil.
    GLuint front = all_bits_one<GLuint>();

    /// Mask for back face stencil.
    GLuint back = all_bits_one<GLuint>();
};

// data usage ======================================================================================================= //

/// The expected usage of the data.
enum class GLUsage {
    DYNAMIC_DRAW, ///< Written many times, read many times by the GPU (default)
    DYNAMIC_READ, ///< Written many times, read many times from the application
    DYNAMIC_COPY, ///< Written many times, read many times from the application as a source for new writes
    STATIC_DRAW,  ///< Written once, read many times from the GPU
    STATIC_READ,  ///< Written once, read many times from the application
    STATIC_COPY,  ///< Written once, read many times from the application as a source for new writes
    STREAM_DRAW,  ///< Written once, read only a few times by the GPU
    STREAM_READ,  ///< Written once, read only a few times from the application
    STREAM_COPY,  ///< Written once, read only a few times from the application as a source for new writes
    DEFAULT = DYNAMIC_DRAW,
};

/// Converts the GLUsage type into an OpenGL enum.
GLenum get_gl_usage(const GLUsage usage);

// gl utils ========================================================================================================= //

/// Calculate a GLvoid buffer offset.
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(nullptr) + offset; }

// data types ======================================================================================================= //

/// Returns the OpenGL type identifier for selected C++ types.
inline constexpr GLenum to_gl_type(const GLbyte&) noexcept { return GL_BYTE; }
inline constexpr GLenum to_gl_type(const GLubyte&) noexcept { return GL_UNSIGNED_BYTE; }
inline constexpr GLenum to_gl_type(const GLshort&) noexcept { return GL_SHORT; }
inline constexpr GLenum to_gl_type(const GLushort&) noexcept { return GL_UNSIGNED_SHORT; }
inline constexpr GLenum to_gl_type(const GLint&) noexcept { return GL_INT; }
inline constexpr GLenum to_gl_type(const GLuint&) noexcept { return GL_UNSIGNED_INT; }
inline constexpr GLenum to_gl_type(const half&) noexcept { return GL_HALF_FLOAT; }
inline constexpr GLenum to_gl_type(const GLfloat&) noexcept { return GL_FLOAT; }

/// Returns the human readable name to an OpenGL type enum.
/// @param type     OpenGL type enum value.
const char* get_gl_type_name(GLenum type);

// opengl error handling ============================================================================================ //

/// Specialized exception that logs an error from OpenGL.
NOTF_EXCEPTION_TYPE(OpenGLError);

namespace detail {

/// Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line             Line at which the error occurred.
/// @param file             File in which the error occurred.
/// @throws opengl_error    Error containing the reported OpenGL error message.
void check_gl_error(unsigned int line, const char* file);

} // namespace detail

/// Convenience macro calling `check_gl_error` after the invocation of the given function.
#ifdef NOTF_DEBUG
#define NOTF_CHECK_GL(A) \
    (A);                 \
    ::notf::detail::check_gl_error(__LINE__, notf::filename_from_path(__FILE__))
#else
#define NOTF_CHECK_GL(A) (A)
#endif

/// Clear all OpenGL errors that have occured since the application start or the last call to `clear_gl_errors`.
void clear_gl_errors();

NOTF_CLOSE_NAMESPACE
