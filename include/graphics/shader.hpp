#pragma once

#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

class RenderContext;

/** Manages the compilation, runtime functionality and resources of an OpenGL shader program. */
class Shader final {

public: // static methods
    /** Builds an OpenGL Shader from sources.
     * @param context           Render Context in which the Shader lives.
     * @param shader_name       Name of this Shader.
     * @param vertex_shader     Vertex shader source.
     * @param fragment_shader   Fragment shader source.
     * @return                  Shader instance, is empty if the compilation failed.
     */
    static Shader build(RenderContext* context,
                        const std::string& name,
                        const std::string& vertex_shader_source,
                        const std::string& fragment_shader_source);

protected: // constructor
    /** Value Constructor.
     * @param id        OpenGL Shader program ID.
     * @param context   Render Context in which the Shader lives.
     * @param name      Human readable name of the Shader.
     */
    Shader(const GLuint id, RenderContext* context, const std::string name);

public: // methods
    /** Default constructor, in case something goes wrong in `build`. */
    Shader();

    /** Move Constructor*/
    Shader(Shader&& other);

    /** Move assignment. */
    Shader& operator=(Shader&& other);

    // no copy or assignment
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /** Destructor */
    ~Shader();

    /** The OpenGL ID of the Shader program. */
    GLuint get_id() const { return m_id; }

    /** Checks if the Shader is valid. */
    bool is_valid() const { return m_id != 0; }

    /** The name of this Shader. */
    const std::string& get_name() const { return m_name; }

    /** Tells OpenGL to use this Shader. */
    void use();

private: // fields
    /** ID of the shader program. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    RenderContext* m_render_context;

    /** The name of this Shader. */
    std::string m_name;
};

} // namespace notf
