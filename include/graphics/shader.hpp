#pragma once

#include <memory>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

class RenderContext_Old;

//*********************************************************************************************************************/

/** Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 *
 * Shader and RenderContext
 * =========================
 * A Shader needs a valid RenderContext (which in turn refers to an OpenGL context), since the Shader class itself
 * only stores the OpenGL ID of the program.
 * You create a Shader by calling `RenderContext::build_shader(name, vert, frag)`, which builds the Shader and attaches
 * the RenderContext to it.
 * The return value is a shared pointer, which you own.
 * However, the RenderContext does keep a weak pointer to the Shader and will deallocate it when it is itself removed.
 * In this case, the remaining Shader will become invalid and you'll get a warning message.
 * In a well-behaved program, all Shader should have gone out of scope by the time the RenderContext is destroyed.
 * This behaviour is similar to the handling of Texture2s.
 */
class Shader {

    friend class RenderContext_Old;
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
    static std::shared_ptr<Shader> build(RenderContext_Old* context, const std::string& name,
                                         const std::string& vertex_shader_source,
                                         const std::string& fragment_shader_source);

protected: // constructor
    /** Value Constructor.
     * @param id        OpenGL Shader program ID.
     * @param context   Render Context in which the Shader lives.
     * @param name      Human readable name of the Shader.
     */
    Shader(const GLuint id, RenderContext_Old* context, const std::string name);

public: // methods
    // no copy or assignment
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    /** Destructor */
    ~Shader();

    /** The OpenGL ID of the Shader program. */
    GLuint get_id() const { return m_id; }

    /** Checks if the Shader is valid.
     * A Shader should always be valid - the only way to get an invalid one is to remove the RenderContext while still
     * holding on to shared pointers of a Shader that lived in the removed RenderContext.
     */
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
    RenderContext_Old* m_render_context;

    /** The name of this Shader. */
    const std::string m_name;
};

} // namespace notf
