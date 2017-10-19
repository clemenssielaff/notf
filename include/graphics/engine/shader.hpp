#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "common/meta.hpp"
#include "graphics/engine/gl_forwards.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(class, Shader);

template <typename Real, FWD_ENABLE_IF_REAL(Real)>
struct _RealVector2;
using Vector2f = _RealVector2<float, true>;

template <typename Real, bool SIMD_SPECIALIZATION, FWD_ENABLE_IF_REAL(Real)>
struct _RealVector4;
using Vector4f = _RealVector4<float, false, true>;

template <typename Real, bool SIMD_SPECIALIZATION, FWD_ENABLE_IF_REAL(Real)>
struct _Xform3;
using Xform3f = _Xform3<float, false, true>;

class GraphicsContext;

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** Manages the compilation, runtime functionality and resources of an OpenGL shader program.
 *
 * Shader and GraphicsContext
 * ==========================
 * A Shader needs a valid GraphicsContext (which in turn refers to an OpenGL context), since the Shader class itself
 * only stores the OpenGL ID of the program.
 * You create a Shader by calling `GraphicsContext::build_shader(name, vert, frag)`, which builds the Shader and
 * attaches the GraphicsContext to it.
 * The return value is a shared pointer, which you own.
 * However, the GraphicsContext does keep a weak pointer to the Shader and will deallocate it when it is itself removed.
 * In this case, the remaining Shader will become invalid and you'll get a warning message.
 * In a well-behaved program, all Shader should have gone out of scope by the time the GraphicsContext is destroyed.
 * This behaviour is similar to the handling of Texture2s.
 */
class Shader : public std::enable_shared_from_this<Shader> {

    friend class GraphicsContext; // creates and finally invalidates all of its Shaders when it is destroyed

public: // types ******************************************************************************************************/
    /** Information about a variable (attribute or uniform) of this shader. */
    struct Variable {
        /** Location of the variable, used to address the variable in the OpenGL shader program. */
        GLint location;

        /** Index of the variable - is not the same as its location. */
        GLuint index;

        /** Type of the variable.
         * See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
         */
        GLenum type;

        /** Number of elements in the variable in units of type.
         * Is always >=1 and only >1 if the variable is an array.
         */
        GLint size;

        /** The name of the variable. */
        std::string name;
    };

    //*****************************************************************************************************************/
    //*****************************************************************************************************************/

    /** Shader scope RAII helper. */
    struct Scope {

        /** Constructor, binds the shader. */
        Scope(Shader* shader);

        /** Move without re-binding. */
        Scope(Scope&& other)
            : m_shader(other.m_shader)
        {
            other.m_shader = nullptr;
        }

        /** Destructor, unbinds the shader again. */
        ~Scope();

        /** Bound shader. */
        Shader* m_shader;
    };
    friend struct Scope;

public: // static methods *********************************************************************************************/
    /** Loads a new OpenGL ES Shader from shader files.
     * @param context               Render Context in which the Shader lives.
     * @param shader_name           Name of this Shader.
     * @param vertex_shader_file    Path to vertex shader.
     * @param fragment_shader_file  Path to fragment shader.
     * @param geometry_shader_file  Path to geometry shader.
     * @throws std::runtime_error   If one of the files could not be loaded.
     * @throws std::runtime_error   If the compilation / linking failed.
     * @return                      Shader instance, is empty if the compilation failed.
     */
    static std::shared_ptr<Shader> load(GraphicsContext& context, const std::string& name,
                                        const std::string& vertex_shader_file,
                                        const std::string& fragment_shader_file,
                                        const std::string& geometry_shader_file = "");

    /** Builds a new OpenGL ES Shader from sources.
     * @param context                   Render Context in which the Shader lives.
     * @param shader_name               Name of this Shader.
     * @param vertex_shader_source      Vertex shader source.
     * @param fragment_shader_source    Fragment shader source.
     * @param geometry_shader_source    Geometry shader source.
     * @throws std::runtime_error       If the compilation / linking failed.
     * @return                          Shader instance.
     */
    static std::shared_ptr<Shader> build(GraphicsContext& context, const std::string& name,
                                         const std::string& vertex_shader_source,
                                         const std::string& fragment_shader_source,
                                         const std::string& geometry_shader_source = "");

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

    /** A scope object that pushes this shader onto the stack and pops it on destruction. */
    Scope scope() { return Scope(this); }

    /** The OpenGL ID of the Shader program. */
    GLuint id() const { return m_id; }

    /** Checks if the Shader is valid.
     * A Shader should always be valid - the only way to get an invalid one is to remove the GraphicsContext while still
     * holding on to shared pointers of a Shader that lived in the removed GraphicsContext.
     */
    bool is_valid() const { return m_id != 0; }

    /** The name of this Shader. */
    const std::string& name() const { return m_name; }

    /** Updates the value of a uniform in the shader.
     * @throws std::runtime_error   If the uniform cannot be found.
     * @throws std::runtime_error   If the value type and the uniform type are not compatible.
     */
    template <typename T>
    void set_uniform(const std::string&, const T&)
    {
        static_assert(always_false_t<T>{}, "No overload for value type in notf::Shader::set_uniform");
    }

    /** Returns the location of the attribute with the given name.
     * @throws std::runtime_error   If there is no attribute with the given name in this shader.
     */
    GLuint attribute(const std::string& name) const;

    /** All attribute variables. */
    const std::vector<Variable>& attributes() { return m_attributes; }

#ifdef _DEBUG
    /** Checks whether the shader can execute in the current OpenGL state.
     * Is expensive and should only be used for debugging!
     * A validation report is logged, regardless whether the validation succeeded or not.
     * @return  True if the validation succeeded.
     */
    bool validate_now() const;
#endif

    // TODO: cache compiled shader binaries next to their text files (like python?)

private: // methods for GraphicsContext *******************************************************************************/
    /** Returns the uniform with the given name.
     * @throws std::runtime_error   If there is no uniform with the given name in this shader.
     */
    const Variable& _uniform(const std::string& name) const;

    /** Deallocates the Shader data and invalidates the Shader. */
    void _deallocate();

private: // fields ****************************************************************************************************/
    /** ID of the shader program. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    GraphicsContext& m_graphics_context;

    /** The name of this Shader. */
    const std::string m_name;

    /** All uniforms of this shader.
     * Uniforms remain constant throughout the shader (modelview matrix and texture samplers for example).
     */
    std::vector<Variable> m_uniforms;

    /** All attributes of this shader.
     * Attributes may change during shader execution across different shader stages (per-vertex attributes for example).
     */
    std::vector<Variable> m_attributes;
};

template <>
void Shader::set_uniform(const std::string&, const int& value);

template <>
void Shader::set_uniform(const std::string&, const unsigned int& value);

template <>
void Shader::set_uniform(const std::string&, const float& value);

template <>
void Shader::set_uniform(const std::string&, const Vector2f& value);

template <>
void Shader::set_uniform(const std::string&, const Vector4f& value);

template <>
void Shader::set_uniform(const std::string&, const Xform3f& value);

} // namespace notf
