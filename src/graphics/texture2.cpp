#include "graphics/texture2.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

#include "assert.h"

#include "common/color.hpp"
#include "common/enum.hpp"
#include "common/log.hpp"
#include "common/size2.hpp"
#include "common/warnings.hpp"
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
        return GL_LINEAR_MIPMAP_LINEAR; // trilinear filtering
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

//********************************************************************************************************************

namespace notf {

const Texture2::Args Texture2::s_default_args = {};

Texture2::Scope::Scope(Texture2* texture)
    : m_texture(texture)
{
    m_texture->m_graphics_context.push_texture(m_texture->shared_from_this());
}

Texture2::Scope::~Scope()
{
    if (m_texture) {
        m_texture->m_graphics_context.pop_texture();
    }
}

std::shared_ptr<Texture2> Texture2::load_image(GraphicsContext& context, const std::string file_path,
                                               const Args& args)
{
    if (!context.is_current()) {
        throw_runtime_error(string_format(
            "Cannot load texture \"%s\" with a context that is not current",
            file_path.c_str()));
    }

    std::vector<uchar> image_data;
    Size2i image_size;
    Texture2::Format texture_format;
    GLenum gl_format       = 0;
    GLenum internal_format = 0;
    GLint alignment        = 0;
    GLsizei image_length   = 0;
    int image_bytes        = 0;

    // load the texture from file
    if (args.codec == Codec::RAW) {
        RawImage image(file_path);
        if (!image) {
            return {};
        }

        image_size.width  = image.width();
        image_size.height = image.height();
        image_bytes       = image.channels();
        image_data        = std::vector<uchar>(image.data(), image.data() + (image_size.area() * image_bytes));

        if (image_bytes == 1) { // grayscale
            gl_format       = GL_RED;
            internal_format = GL_R8;
            texture_format  = Texture2::Format::GRAYSCALE;
            alignment       = 1;
        }
        else if (image_bytes == 3) { // color
            gl_format       = GL_RGB;
            internal_format = GL_SRGB8;
            texture_format  = Texture2::Format::RGB;
            alignment       = 4;
        }
        else if (image_bytes == 4) { // color + alpha
            gl_format       = GL_RGBA;
            internal_format = GL_SRGB8_ALPHA8;
            texture_format  = Texture2::Format::RGBA;
            alignment       = 4;
        }
        else {
            throw_runtime_error(string_format(
                "Cannot load texture with %i bytes per pixel (must be 1, 3 or 4)", image_bytes));
        }
    }
    else if (args.codec == Codec::ASTC) {
        std::ifstream image_file(file_path, std::ios::in | std::ios::binary);
        if (!image_file.good()) {
            throw_runtime_error(string_format("Failed to read texture file: %s", file_path.c_str()));
        }
        image_data = std::vector<uchar>(std::istreambuf_iterator<char>(image_file),
                                        std::istreambuf_iterator<char>());

        image_size     = {1024, 1024};
        texture_format = Texture2::Format::RGBA;
        alignment      = 4;
        image_bytes    = 4;

        internal_format = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6;
        image_length    = static_cast<GLsizei>(ceil(image_size.width / 6.)) * static_cast<GLsizei>(ceil(image_size.height / 6.)) * 16;

        // TODO: 'header' reader for ASTC files, we need the image size, the block size and the format (rgb/argb)
    }
    else {
        assert(0);
    }

    // load the texture into OpenGL
    GLuint id = 0;
    glGenTextures(1, &id);
    assert(id);
    glBindTexture(GL_TEXTURE_2D, id);
    check_gl_error();

    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image_size.width);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, image_size.height);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    check_gl_error();

    if (args.make_immutable) {
        // immutable texture
        const GLsizei max_levels = static_cast<GLsizei>(floor(log2(max(image_size.width, image_size.height)))) + 1;
        const GLsizei levels     = args.generate_mipmaps ? max_levels : 1;
        glTexStorage2D(GL_TEXTURE_2D, levels, internal_format, image_size.width, image_size.height);

        if (args.codec == Codec::RAW) {
            glTexSubImage2D(
                GL_TEXTURE_2D, /* level= */ 0, /* xoffset= */ 0, /* yoffset= */ 0,
                image_size.width, image_size.height, gl_format, GL_UNSIGNED_BYTE, &image_data.front());
        }
        else if (args.codec == Codec::ASTC) {
            glCompressedTexSubImage2D(
                GL_TEXTURE_2D, /* level= */ 0, /* xoffset= */ 0, /* yoffset= */ 0,
                image_size.width, image_size.height, internal_format, image_length, &image_data.front());
        }
        else {
            assert(0);
        }
#ifdef _DEBUG
        {
            GLint is_immutable = 0;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &is_immutable);
            assert(is_immutable);
        }
#endif
    }
    else {
        // mutable texture
        if (args.codec == Codec::RAW) {
            glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, static_cast<GLint>(internal_format),
                         image_size.width, image_size.height, BORDER, gl_format, GL_UNSIGNED_BYTE, &image_data.front());
        }
        else if (args.codec == Codec::ASTC) {
            glCompressedTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, image_size.width, image_size.height,
                                   BORDER, image_length, &image_data.front());
        }
        else {
            assert(0);
        }
    }
    check_gl_error();

    // highest quality mip-mapping by default
    if (args.generate_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(args.min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(args.mag_filter));
    check_gl_error();

    // repeat wrap by default
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(args.wrap_horizontal));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(args.wrap_vertical));
    check_gl_error();

    // make texture anisotropic, if requested and available
    if (args.anisotropy > 1.f && context.extensions().anisotropic_filter) {
        GLfloat highest_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &highest_anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(args.anisotropy, highest_anisotropy));
    }

    { // log success
#if NOTF_LOG_LEVEL > NOTF_LOG_LEVEL_INFO
        static const std::string grayscale = "grayscale";
        static const std::string rgb       = "rgb";
        static const std::string rgba      = "rgba";

        const std::string* format_name;
        if (image_bytes == 1) {
            format_name = &grayscale;
        }
        else if (image_bytes == 3) {
            format_name = &rgb;
        }
        else { // color + alpha
            assert(image_bytes == 4);
            format_name = &rgba;
        }

        log_trace << "Loaded " << image_size.width << "x" << image_size.height << " " << *format_name
                  << " OpenGL texture with ID: " << id << " from: " << file_path;
#endif
    }

    // return the loaded texture on success
    std::shared_ptr<Texture2> texture = Texture2::_create(
        id, context, std::move(file_path), image_size.width, image_size.height, texture_format);
    context.m_textures.emplace_back(texture);
    return texture;
}

std::shared_ptr<Texture2> Texture2::create_empty(GraphicsContext& context, const std::string name, const Size2i& size,
                                                 const Args& args)
{
    if (!context.is_current()) {
        throw_runtime_error(string_format(
            "Cannot create empty texture \"%s\" with a context that is not current",
            name.c_str()));
    }

    if (!size.is_valid()) {
        throw_runtime_error(string_format(
            "Cannot create a texture with an invalid size: %s",
            size));
    }

    // create the data
    std::vector<uchar> data(static_cast<size_t>(size.area()), 0);

    // translate to OpenGL format
    GLenum gl_format      = 0;
    GLint internal_format = 0;
    GLint alignment       = 0;
    switch (args.format) {
    case Format::GRAYSCALE:
        gl_format       = GL_RED;
        internal_format = GL_R8;
        alignment       = 1;
        break;
    case Format::RGB:
        gl_format       = GL_RGB;
        internal_format = GL_SRGB8;
        alignment       = 4;
        break;
    case Format::RGBA:
        gl_format       = GL_RGBA;
        internal_format = GL_SRGB8_ALPHA8;
        alignment       = 4;
        break;
    }

    // create the atlas texture
    GLuint id = 0;
    glGenTextures(1, &id);
    assert(id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, size.width);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, size.height);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, size.width, size.height,
                 BORDER, gl_format, GL_UNSIGNED_BYTE, &data[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(args.min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(args.mag_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(args.wrap_horizontal));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(args.wrap_vertical));
    check_gl_error();

    // return the loaded texture on success
    std::shared_ptr<Texture2> texture = Texture2::_create(
        id, context, std::move(name), size.width, size.height, args.format);
    context.m_textures.emplace_back(texture);
    return texture;
}

std::shared_ptr<Texture2> Texture2::_create(const GLuint id, GraphicsContext& context, const std::string name,
                                            const GLint width, const GLint height, const Format format)
{
#ifdef _DEBUG
    return std::shared_ptr<Texture2>(new Texture2(id, context, name, width, height, format));
#else
    struct make_shared_enabler : public Texture2 {
        make_shared_enabler(const GLuint id, GraphicsContext& context, const std::string name,
                            const GLint width, const GLint height, const Format format)
            : Texture2(id, context, name, width, height, format) {}
        PADDING(3)
    };
    return std::make_shared<make_shared_enabler>(id, context, std::move(name), width, height, format);
#endif
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

void Texture2::set_min_filter(const MinFilter filter)
{
    m_graphics_context.push_texture(shared_from_this());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(filter));
    m_min_filter = filter;
    m_graphics_context.pop_texture();
    check_gl_error();
}

void Texture2::set_mag_filter(const MagFilter filter)
{
    m_graphics_context.push_texture(shared_from_this());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(filter));
    m_mag_filter = filter;
    m_graphics_context.pop_texture();
    check_gl_error();
}

void Texture2::set_wrap_x(const Wrap wrap)
{
    m_graphics_context.push_texture(shared_from_this());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(wrap));
    m_wrap_x = wrap;
    m_graphics_context.pop_texture();
    check_gl_error();
}

void Texture2::set_wrap_y(const Wrap wrap)
{
    m_graphics_context.push_texture(shared_from_this());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(wrap));
    m_wrap_y = wrap;
    m_graphics_context.pop_texture();
    check_gl_error();
}

void Texture2::fill(const Color& color)
{
    if (!m_graphics_context.is_current()) {
        throw_runtime_error(string_format(
            "Cannot fill texture \"%s\" with a context that is not current",
            m_name.c_str()));
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
    default:
        fill_color = Color::black();
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
    check_gl_error();
}

void Texture2::_deallocate()
{
    if (m_id) {
        assert(m_graphics_context.is_current());
        glDeleteTextures(1, &m_id);
        check_gl_error();
        log_trace << "Deleted OpenGL texture with ID: " << m_id;
    }
    m_id = 0;
}

} // namespace notf
