#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
     * \brief Builds an OpenGL Shader instance from source files.
     * \param vertex_shader_path    Path to a vertex shader source file.
     * \param fragment_shader_path  Path to a fragment shader source file.
     * \param geometry_shader_path  (optional) Path to a geometry shader source file.
     * \return Shader instance, is empty if the compilation failed.
     */
    static std::shared_ptr<Shader> build(
        const std::string& vertex_shader_path,
        const std::string& fragment_shader_path,
        const std::string& geometry_shader_path = "");

protected: // methods
    /**
     * \brief Value constructor.
     * \param id    OpenGL shader ID.
     */
    explicit Shader(GLuint id)
        : m_id(id)
    {
    }

public: // methods
    /// no copy / assignment
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /**
     * \brief Destructor.
     */
    ~Shader();

    /**
     * \brief The OpenGL ID of this Shader.
     */
    GLuint get_id() const { return m_id; }

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

    /**
     * \brief Sets an uniform 2D GLM vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec2& value);

    /**
     * \brief Sets an uniform 3D GLM vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec3& value);

    /**
     * \brief Sets an uniform 3D GLM vector in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::vec4& value);

    /**
     * \brief Sets an uniform 4x4 GLM matrix in this Shader.
     * \param name  Name of the uniform to set.
     * \param value New value.
     */
    void set_uniform(const GLchar* name, const glm::mat4& matrix);

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
