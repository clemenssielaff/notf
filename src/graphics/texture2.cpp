#include "graphics/texture2.hpp"

#include "assert.h"

#include "common/color.hpp"
#include "common/enum.hpp"
#include "common/log.hpp"
#include "common/size2.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/raw_image.hpp"

namespace { // anonymous
using namespace notf;

// must be zero - as seen on: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
static const GLint BORDER = 0;

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

struct Texture2::make_shared_enabler : public Texture2 {
    template <typename... Args>
    make_shared_enabler(Args&&... args)
        : Texture2(std::forward<Args>(args)...) {}
};

std::shared_ptr<Texture2> Texture2::load_image(GraphicsContext& context, const std::string file_path)
{
    // load the texture from file
    RawImage image(file_path);
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
    context.make_current();
    glGenTextures(1, &id);
    assert(id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, image.get_bytes_per_pixel());
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image.get_width());
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, image.get_width(), image.get_height(),
                 BORDER, gl_format, GL_UNSIGNED_BYTE, image.get_data());

    // repeat wrap by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // highest quality mip-mapping by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    check_gl_error();

    // log success
    {
#if NOTF_LOG_LEVEL > NOTF_LOG_LEVEL_INFO
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
                  << " OpenGL texture with ID: " << id << " from: " << file_path;
#endif
    }

    // return the loaded texture on success
    std::shared_ptr<Texture2> texture = std::make_shared<make_shared_enabler>(
        id, context, std::move(file_path), image.get_width(), image.get_height(), texture_format);
    context.m_textures.emplace_back(texture);
    return texture;
}

std::shared_ptr<Texture2> Texture2::create_empty(GraphicsContext& context, const std::string name,
                                                 const Size2i& size, const Format format)
{
    if (!size.is_valid()) {
        log_critical << "Cannot create a Texture2 with an invalid size";
        return {};
    }

    // create the data
    std::vector<uchar> data(static_cast<size_t>(size.area()), 0);

    // translate to OpenGL format
    GLenum gl_format;
    GLint internal_format;
    switch (format) {
    case Format::GRAYSCALE:
        gl_format       = GL_RED;
        internal_format = GL_R8;
        break;
    case Format::RGB:
        gl_format       = GL_RGB;
        internal_format = GL_RGB8;
        break;
    case Format::RGBA:
        gl_format       = GL_RGBA;
        internal_format = GL_RGBA8;
        break;
    default:
        assert(0);
    }

    // create the atlas texture
    GLuint id = 0;
    context.make_current();
    glGenTextures(1, &id);
    assert(id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<int>(to_number(format)));
    glPixelStorei(GL_UNPACK_ROW_LENGTH, size.width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, size.width, size.height,
                 BORDER, gl_format, GL_UNSIGNED_BYTE, &data[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_gl_error();

    // return the loaded texture on success
    std::shared_ptr<Texture2> texture = std::make_shared<make_shared_enabler>(
        id, context, std::move(name), size.width, size.height, format);
    context.m_textures.emplace_back(texture);
    return texture;
}

void Texture2::unbind()
{
    glBindTexture(GL_TEXTURE_2D, GL_ZERO);
}

Texture2::Texture2(const GLuint id, GraphicsContext& context, const std::string name,
                   const GLint width, const GLint height, const Format format)
    : m_id(id)
    , m_graphics_context(context)
    , m_name(name)
    , m_width(width)
    , m_height(height)
    , m_format(format)
    , m_min_filter(MinFilter::LINEAR_MIPMAP_LINEAR)
    , m_mag_filter(MagFilter::LINEAR)
    , m_wrap_x(Wrap::REPEAT)
    , m_wrap_y(Wrap::REPEAT)
{
    if (m_width <= 0 || m_height <= 0) {
        log_critical << "Cannot create a Texture with zero or negative area";
        _deallocate();
    }
}

Texture2::~Texture2()
{
    _deallocate();
}

bool Texture2::bind() const
{
    if (is_valid()) {
        m_graphics_context.make_current();
        m_graphics_context._bind_texture(m_id);
        return true;
    }
    else {
        log_critical << "Cannot bind invalid Texture \"" << m_name << "\"";
        return false;
    }
}

void Texture2::set_min_filter(const MinFilter filter)
{
    if (bind()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(filter));
        m_min_filter = filter;
    }
}

void Texture2::set_mag_filter(const MagFilter filter)
{
    if (bind()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(filter));
        m_mag_filter = filter;
    }
}

void Texture2::set_wrap_x(const Wrap wrap)
{
    if (bind()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(wrap));
        m_wrap_x = wrap;
    }
}

void Texture2::set_wrap_y(const Wrap wrap)
{
    if (bind()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(wrap));
        m_wrap_y = wrap;
    }
}

void Texture2::fill(const Color& color)
{
    if (!bind()) {
        return;
    }

    // adjust the color to the texture
    Color fill_color;
    switch (m_format) {
    case Format::GRAYSCALE:
        fill_color = color.to_greyscale();
        break;
    case Format::RGB:
        fill_color = color.premultiplied();
        break;
    case Format::RGBA:
        fill_color = color;
        break;
    }
    const uchar r = static_cast<uchar>(roundf(fill_color.r * 255.f));
    const uchar g = static_cast<uchar>(roundf(fill_color.g * 255.f));
    const uchar b = static_cast<uchar>(roundf(fill_color.b * 255.f));
    const uchar a = static_cast<uchar>(roundf(fill_color.a * 255.f));

    // create the source buffer and copy it into the texture
    if (m_format == Format::GRAYSCALE) {
        const std::vector<uchar> buffer(static_cast<size_t>(m_width * m_height * to_number(m_format)), r);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, &buffer[0]);
    }
    else if (m_format == Format::RGB) {
        std::vector<uchar> buffer;
        buffer.reserve(static_cast<size_t>(m_width * m_height * to_number(m_format)));
        for (size_t i = 0; i < static_cast<size_t>(m_width * m_height); ++i) {
            buffer[i * to_number(Format::RGB) + 0] = r;
            buffer[i * to_number(Format::RGB) + 1] = g;
            buffer[i * to_number(Format::RGB) + 2] = b;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, &buffer[0]);
    }
    else if (m_format == Format::RGBA) {
        std::vector<uchar> buffer;
        buffer.reserve(static_cast<size_t>(m_width * m_height * to_number(m_format)));
        for (size_t i = 0; i < static_cast<size_t>(m_width * m_height); ++i) {
            buffer[i * to_number(Format::RGBA) + 0] = r;
            buffer[i * to_number(Format::RGBA) + 1] = g;
            buffer[i * to_number(Format::RGBA) + 2] = b;
            buffer[i * to_number(Format::RGBA) + 3] = a;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &buffer[0]);
    }
}

void Texture2::_deallocate()
{
    if (m_id) {
        m_graphics_context.make_current();
        glDeleteTextures(1, &m_id);
        log_trace << "Deleted OpenGL texture with ID: " << m_id;
        m_id = 0;
    }
}

} // namespace notf
