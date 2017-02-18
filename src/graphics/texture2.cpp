#include "graphics/texture2.hpp"

#include "assert.h"

#include "core/glfw_wrapper.hpp"

#include "common/log.hpp"
#include "graphics/raw_image.hpp"
#include "utils/make_smart_enabler.hpp"

namespace { // anonymous
using namespace notf;

// as seen on: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
static const GLint THIS_VALUE_MUST_BE_ZERO = 0;

GLint wrap_to_gl(const Texture2::Wrap wrap)
{
    switch (wrap) {
    case Texture2::Wrap::REPEAT:
        return GL_REPEAT;
    case Texture2::Wrap::CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case Texture2::Wrap::MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    }
    assert(0);
    return GL_ZERO;
}

GLint minfilter_to_gl(const Texture2::MinFilter filter)
{
    switch (filter) {
    case Texture2::MinFilter::NEAREST:
        return GL_NEAREST;
    case Texture2::MinFilter::LINEAR:
        return GL_LINEAR;
    case Texture2::MinFilter::NEAREST_MIPMAP_NEAREST:
        return GL_NEAREST_MIPMAP_NEAREST;
    case Texture2::MinFilter::NEAREST_MIPMAP_LINEAR:
        return GL_NEAREST_MIPMAP_LINEAR;
    case Texture2::MinFilter::LINEAR_MIPMAP_NEAREST:
        return GL_LINEAR_MIPMAP_NEAREST;
    case Texture2::MinFilter::LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR_MIPMAP_LINEAR;
    }
    assert(0);
    return GL_ZERO;
}

GLint magfilter_to_gl(const Texture2::MagFilter filter)
{
    switch (filter) {
    case Texture2::MagFilter::NEAREST:
        return GL_NEAREST;
    case Texture2::MagFilter::LINEAR:
        return GL_LINEAR;
    }
    assert(0);
    return GL_ZERO;
}

} // namespace anonymous

namespace notf {

std::shared_ptr<Texture2> Texture2::load(const std::string& texture_path)
{
    // load the texture from file
    RawImage image(texture_path);
    if (!image) {
        return {};
    }

    // analyze the texture
    GLenum gl_format      = 0;
    GLint internal_format = 0;
    Texture2::Format texture_format;
    {
        const int bytes = image.get_bytes_per_pixel();
        if (bytes == 1) { // grayscale
            gl_format       = GL_RED;
            internal_format = GL_R8;
            texture_format  = Texture2::Format::GRAYSCALE;
        }
        else if (bytes == 3) { // color
            gl_format       = GL_RGB;
            internal_format = GL_RGB8;
            texture_format  = Texture2::Format::RGB;
        }
        else if (bytes == 4) { // color + alpha
            gl_format       = GL_RGBA;
            internal_format = GL_RGBA8;
            texture_format  = Texture2::Format::RGBA;
        }
        else {
            log_critical << "Cannot load texture with " << bytes << " bytes per pixel (must be 1, 3 or 4)";
            return {};
        }
    }

    // load the texture into OpenGL
    GLuint id = 0;
    glGenTextures(1, &id);
    assert(id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image.get_width());
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, image.get_width(), image.get_height(),
                 THIS_VALUE_MUST_BE_ZERO, gl_format, GL_UNSIGNED_BYTE, image.get_data());

    // repeat wrap by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // highest quality mip-mapping by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // TODO: this is just before mip map generation in nanovg .. does that make a difference?
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    unbind();

    // log success
    {
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_TRACE
        static const std::string grayscale = "grayscale";
        static const std::string rgb       = "rgb";
        static const std::string rgba      = "rgba";

        const int bytes = image.get_bytes_per_pixel();
        const std::string* format_name;
        if (bytes == 1) {
            format_name = &grayscale;
        }
        else if (bytes == 3) {
            format_name = &rgb;
        }
        else { // color + alpha
            assert(bytes == 4);
            format_name = &rgba;
        }

        log_trace << "Loaded " << image.get_width() << "x" << image.get_height() << " " << *format_name
                  << " OpenGL texture with ID: " << id << " from: " << texture_path;
#endif
    }

    // return the loaded texture on success

    return std::make_shared<MakeSmartEnabler<Texture2>>(
        id, static_cast<GLuint>(image.get_width()), static_cast<GLuint>(image.get_height()), texture_format);
}

Texture2::Texture2(const GLuint id, const GLuint width, const GLuint height, const Format format)
    : m_id(id)
    , m_width(width)
    , m_height(height)
    , m_format(format)
    , m_min_filter(MinFilter::LINEAR_MIPMAP_LINEAR)
    , m_mag_filter(MagFilter::LINEAR)
    , m_wrap_x(Wrap::REPEAT)
    , m_wrap_y(Wrap::REPEAT)
{
}

void Texture2::bind() const
{
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture2::unbind()
{
    glBindTexture(GL_TEXTURE_2D, GL_ZERO);
}

Texture2::~Texture2()
{
    glDeleteTextures(1, &m_id);
    log_trace << "Deleted OpenGL texture with ID: " << m_id;
}

void Texture2::set_min_filter(const MinFilter filter)
{
    if (GLint value = minfilter_to_gl(filter)) {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
    }
}

void Texture2::set_mag_filter(const MagFilter filter)
{
    if (GLint value = magfilter_to_gl(filter)) {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
    }
}

void Texture2::set_wrap_x(const Wrap wrap)
{
    if (GLint value = wrap_to_gl(wrap)) {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, value);
    }
}

void Texture2::set_wrap_y(const Wrap wrap)
{
    if (GLint value = wrap_to_gl(wrap)) {
        bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, value);
    }
}

} // namespace notf
