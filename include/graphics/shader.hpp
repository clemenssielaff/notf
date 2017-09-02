#pragma once

#include <memory>
#include <string>

#include "common/meta.hpp"
#include "graphics/gl_forwards.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(class, Shader);

class GraphicsContext;

//*********************************************************************************************************************/

/** Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 *
 * Shader and GraphicsContext
 * ==========================
 * A Shader needs a valid GraphicsContext (which in turn refers to an OpenGL context), since the Shader class itself
 * only stores the OpenGL ID of the program.
 * You create a Shader by calling `GraphicsContext::build_shader(name, vert, frag)`, which builds the Shader and attaches
 * the GraphicsContext to it.
 * The return value is a shared pointer, which you own.
 * However, the GraphicsContext does keep a weak pointer to the Shader and will deallocate it when it is itself removed.
 * In this case, the remaining Shader will become invalid and you'll get a warning message.
 * In a well-behaved program, all Shader should have gone out of scope by the time the GraphicsContext is destroyed.
 * This behaviour is similar to the handling of Texture2s.
 */
class Shader {

    friend class GraphicsContext; // creates and finally invalidates all of its Shaders when it is destroyed

public: // static methods *********************************************************************************************/
    /** Builds a new OpenGL ES Shader from sources.
     * @param context           Render Context in which the Shader lives.
     * @param shader_name       Name of this Shader.
     * @param vertex_shader     Vertex shader source.
     * @param fragment_shader   Fragment shader source.
     * @return                  Shader instance, is empty if the compilation failed.
     */
    static std::shared_ptr<Shader> build(GraphicsContext& context, const std::string& name,
                                         const std::string& vertex_shader_source,
                                         const std::string& fragment_shader_source);

    /** Unbinds any current active Shader. */
    static void unbind();

private: // constructor ***********************************************************************************************/
    /** Value Constructor.
     * @param id        OpenGL Shader program ID.
     * @param context   Render Context in which the Shader lives.
     * @param name      Human readable name of the Shader.
     */
    Shader(const GLuint id, GraphicsContext& context, const std::string name);

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(Shader)

    /** Destructor */
    ~Shader();

    /** The OpenGL ID of the Shader program. */
    GLuint id() const { return m_id; }

    /** Checks if the Shader is valid.
     * A Shader should always be valid - the only way to get an invalid one is to remove the GraphicsContext while still
     * holding on to shared pointers of a Shader that lived in the removed GraphicsContext.
     */
    bool is_valid() const { return m_id != 0; }

    /** The name of this Shader. */
    const std::string& name() const { return m_name; }

    /** Binds this as the current active shader.
     * @throw   std::runtime_error if the shader's graphics context is not current or this shader is invalid.
     */
    void bind();

private: // methods for GraphicsContext *******************************************************************************/
    /** Deallocates the Shader data and invalidates the Shader. */
    void _deallocate();

private: // fields ****************************************************************************************************/
    /** ID of the shader program. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    GraphicsContext& m_graphics_context;

    /** The name of this Shader. */
    const std::string m_name;
};

} // namespace notf
