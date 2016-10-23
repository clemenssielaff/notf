#pragma once

#include <memory>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

/**
 * @brief Manages the loading and setup of an OpenGL texture.
 *
 * The Texture2 object has minimal state, it queries all additional values from the OpenGL texture.
 * Every time you ask for a value via get_*(), OpenGL has to rebind the texture.
 * If this ever becomes a performance bottleneck, we can pull more state into this class.
 */
class Texture2 {

public: // static methods
    /**
     * @brief Loads a texture from a given file.
     * @param texture_path  Path to a texture file.
     * @return Texture2 instance, is empty if the texture could not be loaded.
     */
    static std::shared_ptr<Texture2> load(const std::string& texture_path);

    /**
     * @brief Unbinds any current active GL_TEXTURE_2D.
     */
    static void unbind();

public: // methods
    /// no copy / assignment
    Texture2(const Texture2&) = delete;
    Texture2& operator=(const Texture2&) = delete;

    /**
     * @brief Destructor.
     */
    ~Texture2();

    /**
     * @brief The OpenGL ID of this Texture2.
     */
    GLuint get_id() const { return m_id; }

    /**
     * @brief Width of the loaded image in pixels.
     */
    GLuint get_width() const { return m_width; }

    /**
     * @brief Height of the loaded image in pixels.
     */
    GLuint get_height() const { return m_height; }

    /**
     * @brief Returns the horizonal wrap mode.
     */
    GLint get_wrap_x() const;

    /**
     * @brief Sets a new horizonal wrap mode.
     * @param mode  New mode.
     */
    void set_wrap_x(const GLint mode);

    /**
     * @brief Returns the vertical wrap mode.
     */
    GLint get_wrap_y() const;

    /**
     * @brief Sets a new vertical wrap mode.
     * @param mode  New mode.
     */
    void set_wrap_y(const GLint mode);

    /**
     * @brief Returns the filter mode when the texture pixels are smaller than scren pixels.
     */
    GLint get_filter_min() const;

    /**
     * @brief Sets a new filter mode when the texture pixels are smaller than scren pixels.
     * @param mode  New mode.
     */
    void set_filter_min(const GLint mode);

    /**
     * @brief Returns the filter mode when the texture pixels are larger than scren pixels.
     */
    GLint get_filter_mag() const;

    /**
     * @brief Sets a new filter mode when the texture pixels are larger than scren pixels.
     * @param mode  New mode.
     */
    void set_filter_mag(const GLint mode);

    /**
     * @brief Binds this Texture2 as the current active GL_TEXTURE_2D.
     */
    void bind() const;

protected: // methods
    /**
     * @brief Value constructor.
     * @param id        OpenGL texture ID.
     * @param width     Width of the loaded image in pixels.
     * @param height    Height of the loaded image in pixels.
     */
    explicit Texture2(const GLuint id, const GLuint width, const GLuint height)
        : m_id(id)
        , m_width(width)
        , m_height(height)
    {
    }

private: // fields
    /**
     * @brief OpenGL ID of this Shader.
     */
    GLuint m_id;

    /**
     * @brief Width of the loaded image in pixels.
     */
    GLuint m_width;

    /**
     * @brief Height of the loaded image in pixels.
     */
    GLuint m_height;
};

} // namespace notf
