#pragma once

#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

/**
 * /brief Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 */
class Shader {

public: // enums
    /**
     * @brief Shader stages.
     */
    enum class STAGE : unsigned char {
        INVALID = 0,
        VERTEX,
        FRAGMENT,
        GEOMETRY,
    };

    /**
     * @brief Returns the name of the given Shader stage.
     * @param stage Requested stage.
     * @return Name of the requested stage.
     */
    static const std::string& stage_name(const STAGE stage);

public: // static methods
    /**
     * @brief Builds an OpenGL Shader from sources.
     * @param shader_name      Name of this Shader.
     * @param vertex_shader    Vertex shader source.
     * @param fragment_shader  Fragment shader source.
     * @param geometry_shader  (optional) Geometry shader source.
     * @return Shader instance, is empty if the compilation failed.
     */
    static Shader build(const std::string& name,
                        const std::string& vertex_shader_source,
                        const std::string& fragment_shader_source,
                        const std::string& geometry_shader_source = "");

public: // methods
    /** Move Constructor*/
    Shader(Shader&& other)
        : m_name(std::move(other.m_name))
        , m_id(other.m_id)
    {
        other.m_id   = 0;
    }

    // no copy or assignment
    Shader(const Shader&) = delete;
    Shader& operator=(Shader&&) = delete;
    Shader& operator=(const Shader&) = delete;

    /** Destructor */
    virtual ~Shader();

    /** The name of this Shader. */
    const std::string& get_name() const { return m_name; }

    /** Checks if the Shader is valid. */
    bool is_valid() const { return m_id != 0; }

protected: // methods
    /** Default constructor, in case something goes wrong in `build`. */
    Shader()
        : m_name("UNINITIALIZED")
        , m_id(0)
    {
    }

    Shader(const std::string name, const GLuint id)
        : m_name(std::move(name))
        , m_id(id)
    {
    }

private: // static methods
    /** Compiles a single shader stage from a given source.
     * @param stage     Stage of the Shader represented by the source.
     * @param name      Name of the Shader program (for error messages).
     * @param source    Source to compile.
     * @return OpenGL ID of the shader - is 0 on error.
     */
    static GLuint _compile(STAGE stage, const std::string& name, const std::string& source);

private: // fields
    /** The name of this Shader. */
    std::string m_name;

    /** ID of the shader program. */
    GLuint m_id;
};

} // namespace notf
