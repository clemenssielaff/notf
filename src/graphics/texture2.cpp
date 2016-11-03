#include "graphics/texture2.hpp"

#include "assert.h"

#include <glad/glad.h>

#include "common/log.hpp"
#include "graphics/raw_image.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

std::shared_ptr<Texture2> Texture2::load(const std::string& texture_path)
{
    // load the texture from file
    Image image(texture_path);
    bool success = static_cast<bool>(image);

    // analyze the texture
    GLenum format = 0;
    if (success) {
        const int bytes = image.get_bytes_per_pixel();
        if (bytes == 1) { // grayscale
            format = GL_DEPTH_COMPONENT;
        }
        else if (bytes == 3) { // color
            format = GL_RGB;
        }
        else if (bytes == 4) { // color + alpha
            format = GL_RGBA;
        }
        else {
            log_critical << "Cannot load texture with " << bytes << " bytes per pixel (must be 1, 3 or 4)";
            success = false;
        }
    }

    // load the texture into OpenGL
    GLuint id = 0;
    if (success) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), image.get_width(), image.get_height(),
                     0, format, GL_UNSIGNED_BYTE, image.get_data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // return empty pointer on failure
    if (!success) {
        log_critical << "Failed to load texture from: " << texture_path;
        return {};
    }

    // log success
    {
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_TRACE
        static const std::string grayscale = "grayscale";
        static const std::string rgb = "rgb";
        static const std::string rgba = "rgba";

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
    assert(id);
    assert(image.get_width() >= 0);
    assert(image.get_height() >= 0);
    return std::make_shared<MakeSmartEnabler<Texture2>>(id, static_cast<GLuint>(image.get_width()), static_cast<GLuint>(image.get_height()));
}

void Texture2::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2::~Texture2()
{
    glDeleteTextures(1, &m_id);
    log_trace << "Deleted OpenGL texture with ID: " << m_id;
}

GLint Texture2::get_wrap_x() const
{
    GLint result;
    bind();
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &result);
    return result;
}

void Texture2::set_wrap_x(const GLint mode)
{
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
}

GLint Texture2::get_wrap_y() const
{
    GLint result;
    bind();
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &result);
    return result;
}

void Texture2::set_wrap_y(const GLint mode)
{
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
}

GLint Texture2::get_filter_min() const
{
    GLint result;
    bind();
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &result);
    return result;
}

void Texture2::set_filter_min(const GLint mode)
{
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
}

GLint Texture2::get_filter_mag() const
{
    GLint result;
    bind();
    glGetTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &result);
    return result;
}

void Texture2::set_filter_mag(const GLint mode)
{
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
}

void Texture2::bind() const
{
    glBindTexture(GL_TEXTURE_2D, m_id);
}

} // namespace notf
