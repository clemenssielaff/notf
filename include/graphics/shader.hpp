#pragma once

#include <string>

#include "glm/glm.hpp" // TODO: remove glm in favor of my own vector / matrix classes?
#include "graphics/gl_forwards.hpp"

namespace signal {

/**
 * /brief Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 *
 * /param vertex_shader_path    Path to the vertex shader file.
 * /param fragment_shader_path  Path to the fragment shader file.
 *
 * /return  OpenGL handle for the produced shader, will be zero if the compilation failed.
 */
class Shader {

private: // methods
    /**
     * \brief Value constructor.
     * \param id    OpenGL shader ID.
     */
    explicit Shader(GLuint id)
        : m_id(id)
    {
    }

public: // static methods
    /**
     * \brief Compiles a Shader instance from source files.
     * \param vertex_shader_path    Path to a vertex shader source file.
     * \param fragment_shader_path  Path to a fragment shader source file.
     * \param geometry_shader_path  (optional) Path to a geometry shader source file.
     * \return Shader instace.
     */
    static Shader from_sources(
        const std::string& vertex_shader_path,
        const std::string& fragment_shader_path,
        const std::string& geometry_shader_path = "");

public: // methods
    /**
     * \brief Default constructor.
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
     * \brief Activates this Shader in OpenGL.
     * \return This Shader.
     */
    Shader& use();

    /*
     * \brief Sets an uniform float in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat value);

    /*
     * \brief Sets an uniform integer in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLint value);

    /*
     * \brief Sets an uniform 2D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y);

    /*
     * \brief Sets an uniform 2D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec2& value);

    /*
     * \brief Sets an uniform 3D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z);

    /*
     * \brief Sets an uniform 3D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec3& value);

    /*
     * \brief Sets an uniform 4D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

    /*
     * \brief Sets an uniform 4D float vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec4& value);

    /*
     * \brief Sets an uniform 4x4 matrix in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::mat4& matrix);

private: // fields
    /**
     * \brief The OpenGL ID of this Shader.
     */
    GLuint m_id;
};

} // namespace signal
