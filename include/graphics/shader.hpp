#pragma once

#include <string>

#include "common/vector2.hpp"
#include "graphics/gl_forwards.hpp"

namespace signal {

/**
 * /brief Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 */
class Shader {

public: // enums
    /**
     * \brief Shader stages.
     */
    enum class STAGE {
        INVALID = 0,
        VERTEX,
        FRAGMENT,
        GEOMETRY,
    };

    /**
     * \brief Returns the name of the given Shader stage.
     * \param stage Requested stage.
     * \return Name of the requested stage.
     */
    static const std::string& stage_name(const STAGE stage);

public: // static methods
    /**
     * \brief Compiles a Shader instance from source files.
     * \param vertex_shader_path    Path to a vertex shader source file.
     * \param fragment_shader_path  Path to a fragment shader source file.
     * \param geometry_shader_path  (optional) Path to a geometry shader source file.
     * \return Shader instance, is empty if the compilation failed.
     */
    static Shader from_sources(
        const std::string& vertex_shader_path,
        const std::string& fragment_shader_path,
        const std::string& geometry_shader_path = "");

private: // methods
    /**
     * \brief Value constructor.
     * \param id    OpenGL shader ID.
     */
    explicit Shader(GLuint id)
        : m_id(id)
    {
    }

public: // methods
    /**
     * \brief Default constructor, produces an empty Shader.
     */
    explicit Shader()
        : m_id(0)
    {
    }

    /**
     * \brief Destructor.
     */
    ~Shader();

    Shader(const Shader&) = delete; // no copy construction
    Shader& operator=(const Shader&) = delete; // no copy assignment

    /**
     * \brief Move constructor.
     * \param other Shader to move from.
     */
    Shader(Shader&& other) noexcept
        : Shader()
    {
        swap(*this, other);
    }

    /**
     * \brief Assignment operator with rvalue.
     * \param other Shader to assign from.
     * \return This Shader.
     */
    Shader& operator=(Shader&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    /**
     * \brief Swap specialization
     * \param first     One of the two Shaders to swap.
     * \param second    The other of the two Shader to swap.
     */
    friend void swap(Shader& first, Shader& second) noexcept
    {
        using std::swap;
        swap(first.m_id, second.m_id);
    }

    /**
     * \brief The OpenGL ID of this Shader.
     */
    GLuint get_id() const { return m_id; }

    /**
     * \brief Checks if this Shader is empty.
     */
    bool is_empty() const { return m_id == 0; }

    /**
     * \brief Activates this Shader in OpenGL.
     * \return This Shader.
     */
    Shader& use();

    /**
     * \brief Sets an uniform float in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat value);

    /**
     * \brief Sets an uniform integer in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLint value);

    /**
     * \brief Sets an uniform 2D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y);

    /**
     * \brief Sets an uniform 2D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const Vector2& value);

    /**
     * \brief Sets an uniform 3D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z);

    /**
     * \brief Sets an uniform 4D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

private: // static methods
    /**
     * \brief Compiles a single shader stage from a given source file.
     * \param stage         Stage of the shader represented by the source.
     * \param shader_path   Source file to compile.
     * \return OpenGL ID of the shader - is 0 on error.
     */
    static GLuint compile(STAGE stage, const std::string& shader_path);

private: // fields
    /**
     * \brief The OpenGL ID of this Shader.
     */
    GLuint m_id;
};

} // namespace signal
