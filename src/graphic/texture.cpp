#include "notf/graphic/texture.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

#include "notf/meta/enum.hpp"
#include "notf/meta/log.hpp"

#include "notf/common/color.hpp"
#include "notf/common/size2.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/raw_image.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

// must be zero - as seen on: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
static const GLint BORDER = 0;

GLint wrap_to_gl(const Texture::Wrap wrap) {
    switch (wrap) {
    case Texture::Wrap::REPEAT: return GL_REPEAT;
    case Texture::Wrap::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
    case Texture::Wrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
    }
    NOTF_ASSERT(false);
    return GL_ZERO;
}

GLint minfilter_to_gl(const Texture::MinFilter filter) {
    switch (filter) {
    case Texture::MinFilter::NEAREST: return GL_NEAREST;
    case Texture::MinFilter::LINEAR: return GL_LINEAR;
    case Texture::MinFilter::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
    case Texture::MinFilter::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
    case Texture::MinFilter::LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
    case Texture::MinFilter::LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR; // trilinear filtering
    }
    NOTF_ASSERT(false);
    return GL_ZERO;
}

GLint magfilter_to_gl(const Texture::MagFilter filter) {
    switch (filter) {
    case Texture::MagFilter::NEAREST: return GL_NEAREST;
    case Texture::MagFilter::LINEAR: return GL_LINEAR;
    }
    NOTF_ASSERT(false);
    return GL_ZERO;
}

GLenum datatype_to_gl(const Texture::DataType type) {
    switch (type) {
    case Texture::DataType::BYTE: return GL_BYTE;
    case Texture::DataType::UBYTE: return GL_UNSIGNED_BYTE;
    case Texture::DataType::SHORT: return GL_SHORT;
    case Texture::DataType::USHORT: return GL_UNSIGNED_SHORT;
    case Texture::DataType::INT: return GL_INT;
    case Texture::DataType::UINT: return GL_UNSIGNED_INT;
    case Texture::DataType::HALF: return GL_HALF_FLOAT;
    case Texture::DataType::FLOAT: return GL_FLOAT;
    case Texture::DataType::USHORT_5_6_5: return GL_UNSIGNED_SHORT_5_6_5;
    }
    NOTF_ASSERT(false);
    return GL_ZERO;
}

#ifdef NOTF_DEBUG
void assert_is_valid(const Texture& texture) {
    if (!texture.is_valid()) {
        NOTF_THROW(ResourceError, "Texture \"{}\" was deallocated! Has TheGraphicsSystem been deleted?",
                   texture.get_name());
    }
}
#else
void assert_is_valid(const Texture&) {} // noop
#endif

void set_texture_parameter(const Texture& texture, const GLenum name, const GLint value) {
    assert_is_valid(texture);
    GraphicsContext::get().bind_texture(&texture, 0);
    NOTF_CHECK_GL(glTexParameteri(texture.get_target(), name, value));
}

} // namespace

// texture ========================================================================================================== //

NOTF_OPEN_NAMESPACE

const Texture::Args Texture::s_default_args = {};

Texture::Texture(const GLuint id, const GLenum target, std::string name, Size2i size, const Format format)
    : m_id(id), m_target(target), m_name(std::move(name)), m_size(std::move(size)), m_format(format) {
    if (!m_size.is_valid() || m_size.get_area() == 0) {
        NOTF_LOG_ERROR("Cannot create a Texture with zero or negative area");
        _deallocate();
    }
}

TexturePtr Texture::create_empty(std::string name, Size2i size, const Args& args) {
    // validate the passed arguments
    if (!size.is_valid()) { NOTF_THROW(ValueError, "Cannot create a texture with an invalid size: {}", size); }

    // translate to OpenGL format
    GLenum gl_format = 0;
    GLint internal_format = 0;
    GLint alignment = 0;
    switch (args.format) {
    case Format::GRAYSCALE:
        gl_format = GL_RED;
        internal_format = GL_R8;
        alignment = 1;
        break;
    case Format::RGB:
        gl_format = GL_RGB;
        internal_format = args.is_linear ? GL_RGB : GL_SRGB8;
        alignment = 4;
        break;
    case Format::RGBA:
        gl_format = GL_RGBA;
        internal_format = args.is_linear ? GL_RGBA : GL_SRGB8_ALPHA8;
        alignment = 4;
        break;
    }

    // create the atlas texture
    GLuint id = 0;
    NOTF_CHECK_GL(glGenTextures(1, &id));
    NOTF_ASSERT(id);
    NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, id));

    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ALIGNMENT, alignment));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ROW_LENGTH, size.width()));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, size.height()));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));

    NOTF_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, size.width(), size.height(), BORDER,
                               gl_format, datatype_to_gl(args.data_type), nullptr));

    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(args.min_filter)));
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(args.mag_filter)));
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(args.wrap_horizontal)));
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(args.wrap_vertical)));

    // return the loaded texture on success
    TexturePtr texture = Texture::_create_shared(id, GL_TEXTURE_2D, name, std::move(size), args.format);
    TheGraphicsSystem::AccessFor<Texture>::register_new(texture);
    ResourceManager::get_instance().get_type<Texture>().set(std::move(name), texture);
    return texture;
}

TexturePtr Texture::load_image(const std::string& file_path, std::string name, const Args& args) {
    std::vector<uchar> image_data;
    Size2i image_size;
    Texture::Format texture_format;
    GLenum gl_format = 0;
    GLenum internal_format = 0;
    GLint alignment = 0;
    GLsizei image_length = 0;
    int image_bytes = 0;

    // load the texture from file
    if (args.codec == Codec::RAW) {
        RawImage image(file_path);
        if (!image) { return {}; }

        image_size.width() = image.get_width();
        image_size.height() = image.get_height();
        image_bytes = image.get_channels();
        image_data = std::vector<uchar>(image.get_data(), image.get_data() + (image_size.get_area() * image_bytes));

        if (image_bytes == 1) { // grayscale
            gl_format = GL_RED;
            internal_format = GL_R8;
            texture_format = Texture::Format::GRAYSCALE;
            alignment = 1;
        } else if (image_bytes == 3) { // color
            gl_format = GL_RGB;
            internal_format = args.is_linear ? GL_RGB : GL_SRGB8;
            texture_format = Texture::Format::RGB;
            alignment = 4;
        } else if (image_bytes == 4) { // color + alpha
            gl_format = GL_RGBA;
            internal_format = args.is_linear ? GL_RGBA : GL_SRGB8_ALPHA8;
            texture_format = Texture::Format::RGBA;
            alignment = 4;
        } else {
            NOTF_THROW(ResourceError, "Cannot load texture with {} bytes per pixel (must be 1, 3 or 4)", image_bytes);
        }
    } else if (args.codec == Codec::ASTC) {
        std::ifstream image_file(file_path, std::ios::in | std::ios::binary);
        if (!image_file.good()) { NOTF_THROW(ResourceError, "Failed to read texture file: \"{}\"", file_path); }
        image_data = std::vector<uchar>(std::istreambuf_iterator<char>(image_file), std::istreambuf_iterator<char>());

        image_size = {1024, 1024};
        texture_format = Texture::Format::RGBA;
        alignment = 4;
        image_bytes = 4;

        internal_format = GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6;
        image_length = static_cast<GLsizei>(ceil(image_size.width() / 6.))
                       * static_cast<GLsizei>(ceil(image_size.height() / 6.)) * 16;

        // TODO: 'header' reader for ASTC files, we need the image size, the block size and the format (rgb/argb)
    } else {
        NOTF_ASSERT(false);
    }

    // load the texture into OpenGL
    GLuint id = 0;
    NOTF_CHECK_GL(glGenTextures(1, &id));
    NOTF_ASSERT(id);
    NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, id));

    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ALIGNMENT, alignment));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ROW_LENGTH, image_size.width()));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, image_size.height()));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));

    if (args.make_immutable) {
        // immutable texture
        const GLsizei max_levels = static_cast<GLsizei>(floor(log2(max(image_size.width(), image_size.height())))) + 1;
        const GLsizei levels = args.create_mipmaps ? max_levels : 1;
        NOTF_CHECK_GL(glTexStorage2D(GL_TEXTURE_2D, levels, internal_format, image_size.width(), image_size.height()));

        if (args.codec == Codec::RAW) {
            NOTF_CHECK_GL(glTexSubImage2D(GL_TEXTURE_2D, /* level= */ 0, /* xoffset= */ 0, /* yoffset= */ 0,
                                          image_size.width(), image_size.height(), gl_format,
                                          datatype_to_gl(args.data_type), &image_data.front()));
        } else if (args.codec == Codec::ASTC) {
            NOTF_CHECK_GL(glCompressedTexSubImage2D(GL_TEXTURE_2D, /* level= */ 0, /* xoffset= */ 0, /* yoffset= */ 0,
                                                    image_size.width(), image_size.height(), internal_format,
                                                    image_length, &image_data.front()));
        } else {
            NOTF_ASSERT(false);
        }
#ifdef NOTF_DEBUG
        {
            GLint is_immutable = 0;
            NOTF_CHECK_GL(glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &is_immutable));
            NOTF_ASSERT(is_immutable);
        }
#endif
    } else {
        // mutable texture
        if (args.codec == Codec::RAW) {
            NOTF_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, /* level= */ 0, static_cast<GLint>(internal_format),
                                       image_size.width(), image_size.height(), BORDER, gl_format,
                                       datatype_to_gl(args.data_type), &image_data.front()));
        } else if (args.codec == Codec::ASTC) {
            NOTF_CHECK_GL(glCompressedTexImage2D(GL_TEXTURE_2D, /* level= */ 0, internal_format, image_size.width(),
                                                 image_size.height(), BORDER, image_length, &image_data.front()));
        } else {
            NOTF_ASSERT(false);
        }
    }

    // highest quality mip-mapping by default
    if (args.create_mipmaps) { NOTF_CHECK_GL(glGenerateMipmap(GL_TEXTURE_2D)); }
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(args.min_filter)));
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(args.mag_filter)));

    // repeat wrap by default
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_to_gl(args.wrap_horizontal)));
    NOTF_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_to_gl(args.wrap_vertical)));

    // make texture anisotropic, if requested and available
    if (args.anisotropy > 1.f && TheGraphicsSystem::get_extensions().anisotropic_filter) {
        GLfloat highest_anisotropy;
        NOTF_CHECK_GL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &highest_anisotropy));
        NOTF_CHECK_GL(
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(args.anisotropy, highest_anisotropy)));
    }

    { // log success
#if NOTF_LOG_LEVEL <= 0
        static const std::string grayscale = "grayscale";
        static const std::string rgb = "rgb";
        static const std::string rgba = "rgba";

        const std::string* format_name;
        if (image_bytes == 1) {
            format_name = &grayscale;
        } else if (image_bytes == 3) {
            format_name = &rgb;
        } else { // color + alpha
            NOTF_ASSERT(image_bytes == 4);
            format_name = &rgba;
        }

        NOTF_LOG_TRACE("Loaded {}x{} {} OpenGL texture with ID: {} from: \"{}\"", image_size.width(),
                       image_size.height(), *format_name, id, file_path);
#endif
    }

    // return the loaded texture on success
    TexturePtr texture = Texture::_create_shared(id, GL_TEXTURE_2D, name, std::move(image_size), texture_format);
    TheGraphicsSystem::AccessFor<Texture>::register_new(texture);
    ResourceManager::get_instance().get_type<Texture>().set(std::move(name), texture);
    return texture;
}

Texture::~Texture() { _deallocate(); }

void Texture::set_min_filter(const MinFilter filter) {
    set_texture_parameter(*this, GL_TEXTURE_MIN_FILTER, minfilter_to_gl(filter));
}

void Texture::set_mag_filter(const MagFilter filter) {
    set_texture_parameter(*this, GL_TEXTURE_MAG_FILTER, magfilter_to_gl(filter));
}

void Texture::set_wrap_x(const Wrap wrap) { set_texture_parameter(*this, GL_TEXTURE_WRAP_S, wrap_to_gl(wrap)); }

void Texture::set_wrap_y(const Wrap wrap) { set_texture_parameter(*this, GL_TEXTURE_WRAP_T, wrap_to_gl(wrap)); }

void Texture::fill(const Color& color) {
    assert_is_valid(*this);

    // adjust the color to the texture
    Color fill_color;
    switch (m_format) {
    case Format::GRAYSCALE: fill_color = color.to_greyscale(); break;
    case Format::RGB: fill_color = color.premultiplied(); break;
    case Format::RGBA: fill_color = color; break;
    }
    const uchar r = static_cast<uchar>(roundf(fill_color.r * 255.f));
    const uchar g = static_cast<uchar>(roundf(fill_color.g * 255.f));
    const uchar b = static_cast<uchar>(roundf(fill_color.b * 255.f));
    const uchar a = static_cast<uchar>(roundf(fill_color.a * 255.f));

    // create the source buffer and copy it into the texture
    if (m_format == Format::GRAYSCALE) {
        const std::vector<uchar> buffer(static_cast<size_t>(m_size.width() * m_size.height() * to_number(m_format)), r);
        NOTF_CHECK_GL(
            glTexImage2D(m_target, 0, GL_R8, m_size.width(), m_size.height(), 0, GL_RED, GL_UNSIGNED_BYTE, &buffer[0]));
    } else if (m_format == Format::RGB) {
        std::vector<uchar> buffer;
        buffer.reserve(static_cast<size_t>(m_size.width() * m_size.height() * to_number(m_format)));
        for (size_t i = 0; i < static_cast<size_t>(m_size.width() * m_size.height()); ++i) {
            buffer[i * to_number(Format::RGB) + 0] = r;
            buffer[i * to_number(Format::RGB) + 1] = g;
            buffer[i * to_number(Format::RGB) + 2] = b;
        }
        NOTF_CHECK_GL(glTexImage2D(m_target, 0, GL_RGB, m_size.width(), m_size.height(), 0, GL_RGB, GL_UNSIGNED_BYTE,
                                   &buffer[0]));
    } else if (m_format == Format::RGBA) {
        std::vector<uchar> buffer;
        buffer.reserve(static_cast<size_t>(m_size.width() * m_size.height() * to_number(m_format)));
        for (size_t i = 0; i < static_cast<size_t>(m_size.width() * m_size.height()); ++i) {
            buffer[i * to_number(Format::RGBA) + 0] = r;
            buffer[i * to_number(Format::RGBA) + 1] = g;
            buffer[i * to_number(Format::RGBA) + 2] = b;
            buffer[i * to_number(Format::RGBA) + 3] = a;
        }
        NOTF_CHECK_GL(glTexImage2D(m_target, 0, GL_RGBA, m_size.width(), m_size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                   &buffer[0]));
    }
}

void Texture::_deallocate() {
    if (m_id.is_valid()) {
        NOTF_CHECK_GL(glDeleteTextures(1, &m_id.value()));
        NOTF_LOG_TRACE("Deleted OpenGL texture with ID: {}", m_id);
        m_id = TextureId::invalid();
    }
}

NOTF_CLOSE_NAMESPACE
