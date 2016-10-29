#include "graphics/texture2.hpp"

#include "assert.h"

#include <glad/glad.h>
#include <stb_image/std_image.h>

#include "common/log.hpp"
#include "utils/smart_enabler.hpp"

namespace { // anonymous

/**
 * @brief Transparency - is used as default border.
 */
static constexpr float transparency[] = {0.0f, 0.0f, 0.0f, 0.0f};

} // namespace anonymous

namespace notf {

std::shared_ptr<Texture2> Texture2::load(const std::string& texture_path)
{
    // load the texture from file
    int width, height, bits;
    unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &bits, 0);
    bool success = static_cast<bool>(data);

    // analyze the texture
    GLenum format = 0;
    if (success) {
        if (bits == 1) { // grayscale
            format = GL_DEPTH_COMPONENT;
        }
        else if (bits == 3) { // color
            format = GL_RGB;
        }
        else if (bits == 4) { // color + alpha
            format = GL_RGBA;
        }
        else {
            log_critical << "Cannot load texture with " << bits << " bits per pixel (must be 1, 3 or 4)";
            success = false;
        }
    }

    // load the texture into OpenGL
    GLuint id = 0;
    if (success) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        //        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, transparency); // TODO: own color class
        //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    stbi_image_free(data);

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

        const std::string* format_name;
        if (bits == 1) {
            format_name = &grayscale;
        }
        else if (bits == 3) {
            format_name = &rgb;
        }
        else { // color + alpha
            assert(bits == 4);
            format_name = &rgba;
        }

        log_trace << "Loaded " << width << "x" << height << " " << *format_name << " OpenGL texture with ID: " << id
                  << " from: " << texture_path;
#endif
    }

    // return the loaded texture on success
    assert(id);
    assert(width >= 0);
    assert(height >= 0);
    return std::make_shared<MakeSmartEnabler<Texture2>>(id, static_cast<GLuint>(width), static_cast<GLuint>(height));
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
