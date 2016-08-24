#pragma once

#include <string>

#include "graphics/gl_forwards.hpp"

namespace signal {

/**
 * \brief Manages the loading and setup of an OpenGL texture.
 */
class Texture2 {

public: // static methods
    /**
     * \brief Loads a texture from a given file.
     * \param texture_path  Path to a texture file.
     * \return Texture2 instance, is empty if the texture could not be loaded.
     */
    static Texture2 load(const std::string& texture_path);

private: // methods
    /**
     * \brief Value constructor.
     * \param id                OpenGL texture ID.
     * \param width             Width of the loaded image in pixels.
     * \param height            Height of the loaded image in pixels.
     */
    explicit Texture2(const GLuint id, const GLuint width, const GLuint height)
        : m_id(id)
        , m_width(width)
        , m_height(height)
    {
    }

public: // methods
    /**
     * \brief Default constructor - produces an empty Texture2.
     */
    explicit Texture2()
        : m_id(0)
        , m_width(0)
        , m_height(0)
    {
    }

    /**
     * \brief Destructor.
     */
    ~Texture2();

    Texture2(const Texture2&) = delete; // no copy construction
    Texture2& operator=(const Texture2&) = delete; // no copy assignment

    /**
     * \brief Move constructor.
     * \param other Texture2 to move from.
     */
    Texture2(Texture2&& other) noexcept
        : Texture2()
    {
        swap(*this, other);
    }

    /**
     * \brief Assignment operator with rvalue.
     * \param other Texture2 to assign from.
     * \return This Texture2.
     */
    Texture2& operator=(Texture2&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    /**
     * \brief Swap specialization
     * \param first     One of the two Textures to swap.
     * \param second    The other of the two Textures to swap.
     */
    friend void swap(Texture2& first, Texture2& second) noexcept
    {
        using std::swap;
        swap(first.m_id, second.m_id);
    }

    /**
     * \brief The OpenGL ID of this Texture2.
     */
    GLuint get_id() const { return m_id; }

    /**
     * \brief Checks if this Texture2 is empty.
     */
    bool is_empty() const { return m_id == 0; }

    /**
     * \brief Width of the loaded image in pixels.
     */
    GLuint get_width() const { return m_width; }

    /**
     * \brief Height of the loaded image in pixels.
     */
    GLuint get_height() const { return m_height; }

    /**
     * \brief Returns the horizonal wrap mode.
     */
    GLint get_wrap_x() const;

    /**
     * \brief Sets a new horizonal wrap mode.
     * \param mode  New mode.
     */
    void set_wrap_x(const GLint mode);

    /**
     * \brief Returns the vertical wrap mode.
     */
    GLint get_wrap_y() const;

    /**
     * \brief Sets a new vertical wrap mode.
     * \param mode  New mode.
     */
    void set_wrap_y(const GLint mode);

    /**
     * \brief Returns the filter mode when the texture pixels are smaller than scren pixels.
     */
    GLint get_filter_min() const;

    /**
     * \brief Sets a new filter mode when the texture pixels are smaller than scren pixels.
     * \param mode  New mode.
     */
    void set_filter_min(const GLint mode);

    /**
     * \brief Returns the filter mode when the texture pixels are larger than scren pixels.
     */
    GLint get_filter_mag() const;

    /**
     * \brief Sets a new filter mode when the texture pixels are larger than scren pixels.
     * \param mode  New mode.
     */
    void set_filter_mag(const GLint mode);

    /**
     * \brief Binds this Texture2 as the current active GL_TEXTURE_2D.
     */
    void bind() const;

public: // static methods
    /**
     * \brief Unbinds any current active GL_TEXTURE_2D.
     */
    static void unbind();

private: // fields
    /**
     * \brief OpenGL ID of this Shader.
     */
    GLuint m_id;

    /**
     * \brief Width of the loaded image in pixels.
     */
    GLuint m_width;

    /**
     * \brief Height of the loaded image in pixels.
     */
    GLuint m_height;
};

} // namespace signal
