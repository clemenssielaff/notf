#pragma once

#include <memory>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

class RenderContext;

/** Manages the compilation, runtime functionality and resources of an OpenGL shader program. */
class Shader {

    friend class RenderContext; // creates and finally invalidates all of its Shaders when it is destroyed

public: // static methods
    /** Unbinds any current active Shader. */
    static void unbind();

private: // static method for RenderContext
    /** Builds a new OpenGL ES Shader from sources.
     * @param context           Render Context in which the Shader lives.
     * @param shader_name       Name of this Shader.
     * @param vertex_shader     Vertex shader source.
     * @param fragment_shader   Fragment shader source.
     * @return                  Shader instance, is empty if the compilation failed.
     */
    static std::shared_ptr<Shader> build(RenderContext* context,
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

    /** Tells OpenGL to use this Shader.
     * @return  False if this Shader is invalid and cannot be bound.
     */
    bool bind();

private: // methods for RenderContext
    /** Deallocates the Shader data and invalidates the Shader. */
    void _deallocate();

private: // fields
    /** ID of the shader program. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    RenderContext* m_render_context;

    /** The name of this Shader. */
    std::string m_name;
};

} // namespace notf
