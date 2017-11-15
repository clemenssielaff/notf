#pragma once

#include <string>
#include <vector>

#include "./gl_forwards.hpp"
#include "./pipeline.hpp"
#include "common/forwards.hpp"

namespace notf {

// TODO: cache compiled shader binaries next to their text files (like python?)

// ===================================================================================================================//

/// @brief Manages the loading and compilation of an OpenGL shader.
///
/// Shader baseclass.
/// Represents a single stage in the shading pipeline.
/// Technically, OpenGL would call this a "program" containing a single "shader" - but in notf you only have shaders and
/// piplines, so we ignore the nomenclature here.
///
/// Shader and GraphicsContext
/// ==========================
/// A Shader needs a valid GraphicsContext (which in turn refers to an OpenGL context), since the Shader class itself
/// only stores the OpenGL ID of the program.
/// Shaders themselves are stored and passed around as shared pointer, which you own.
/// However, the GraphicsContext does keep a weak pointer to the Shader and will deallocate it when it is itself
/// removed. In this case, the remaining Shader will become invalid and you'll get a warning message. In a well-behaved
/// program, all Shader should have gone out of scope by the time the GraphicsContext is destroyed. This behaviour is
/// similar to the handling of Textures.
class Shader : public std::enable_shared_from_this<Shader> {

    friend class GraphicsContext; // creates and finally invalidates all of its Shaders when it is destroyed

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Information about a variable (attribute or uniform) of this shader.
    struct Variable {
        /// @brief Location of the variable, used to address the variable in the OpenGL shader.
        GLint location;

        /// @brief Type of the variable.
        /// See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
        GLenum type;

        /// @brief Number of elements in the variable in units of type.
        /// Is always >=1 and only >1 if the variable is an array.
        GLint size;

        /// @brief The name of the variable.
        std::string name;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// @brief Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param id       OpenGL Shader program ID.
    /// @param name     Context-unique name of this Shader.
    Shader(GraphicsContextPtr& context, const GLuint id, std::string name);

    /// @brief Builds a new OpenGL ES Shader stage from a raw source.
    /// @param context  Graphics Context in which the Shader lives.
    /// @param name     Context-unique name of this Shader.
    /// @param stage    Pipeline stage of the Shader.
    /// @param source   Shader source.
    /// @throws std::runtime_error  If the name is not unique.
    /// @throws std::runtime_error  If the compilation / linking failed.
    /// @return New Shader instance.
    static ShaderPtr
    _build(GraphicsContextPtr& context, std::string name, const Pipeline::Stage stage, const char* source);

    /// @brief Loads a new OpenGL ES Shader stage from a shader file.
    /// @param context  Graphics Context in which the Shader lives.
    /// @param name     Context-unique name of this Shader.
    /// @param stage    Pipeline stage of the Shader.
    /// @param source   Shader source.
    /// @throws std::runtime_error  If the compilation / linking failed.
    /// @return New Shader instance.
    static ShaderPtr
    _load(GraphicsContextPtr& context, std::string name, const Pipeline::Stage stage, const std::string& file);

    // TODO: Shader::load is the wrong abstraction level
    // Shader::build should suffice, the loading of files from disc should be managed by a resource manager that ingests
    // a JSON file

public:
    DISALLOW_COPY_AND_ASSIGN(Shader)

    /// @brief Destructor
    virtual ~Shader();

    /// @brief Pipeline stage of the Shader.
    virtual Pipeline::Stage stage() const = 0;

    /// @brief Graphics Context in which the Texture lives.
    GraphicsContext& context() const { return m_graphics_context; }

    /// @brief The OpenGL ID of the Shader program.
    GLuint id() const { return m_id; }

    /// @brief Checks if the Shader is valid.
    /// A Shader should always be valid - the only way to get an invalid one is to remove the GraphicsContext while
    /// still holding on to shared pointers of a Shader that lived in the removed GraphicsContext.
    bool is_valid() const { return m_id != 0; }

    /// @brief The context-unique name of this Shader.
    const std::string& name() const { return m_name; }

    /// @brief Updates the value of a uniform in the shader.
    /// @throws std::runtime_error   If the uniform cannot be found.
    /// @throws std::runtime_error   If the value type and the uniform type are not compatible.
    template<typename T>
    void set_uniform(const std::string&, const T&)
    {
        static_assert(always_false_t<T>{}, "No overload for value type in notf::Shader::set_uniform");
    }

#ifdef NOTF_DEBUG
    /// @brief Checks whether the shader can execute in the current OpenGL state.
    /// Is expensive and should only be used for debugging!
    /// @return  True if the validation succeeded.
    bool validate_now() const;
#endif

private:
    /// @brief Returns the uniform with the given name.
    /// @throws std::runtime_error   If there is no uniform with the given name in this shader.
    const Variable& _uniform(const std::string& name) const;

    /// @brief Deallocates the Shader data and invalidates the Shader.
    void _deallocate();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Graphics Context in which the Texture lives.
    GraphicsContext& m_graphics_context;

    //// @brief ID of the shader program.
    GLuint m_id;

    /// @brief The context-unique name of this Shader.
    const std::string m_name;

    /// @brief  All uniforms of this shader.
    std::vector<Variable> m_uniforms;
};

template<>
void Shader::set_uniform(const std::string&, const int& value);

template<>
void Shader::set_uniform(const std::string&, const unsigned int& value);

template<>
void Shader::set_uniform(const std::string&, const float& value);

template<>
void Shader::set_uniform(const std::string&, const Vector2f& value);

template<>
void Shader::set_uniform(const std::string&, const Vector4f& value);

template<>
void Shader::set_uniform(const std::string&, const Matrix4f& value);

// ===================================================================================================================//

/// @brief Vertex Shader.
class VertexShader : public Shader {

    friend class Shader;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// @brief Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    VertexShader(GraphicsContextPtr& context, const GLuint program, std::string name);

public:
    static VertexShaderPtr load(GraphicsContextPtr& context, std::string name, const std::string& file)
    {
        return std::static_pointer_cast<VertexShader>(_load(context, std::move(name), Pipeline::Stage::VERTEX, file));
    }

    /// @brief Pipeline stage of the Shader.
    virtual Pipeline::Stage stage() const override final { return Pipeline::Stage::VERTEX; }

    /// @brief Returns the location of the attribute with the given name.
    /// @throws std::runtime_error   If there is no attribute with the given name in this shader.
    GLuint attribute(const std::string& name) const;

    /// @brief All attribute variables.
    const std::vector<Variable>& attributes() { return m_attributes; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief All attributes of this Shader.
    std::vector<Variable> m_attributes;
};

// ===================================================================================================================//

/// @brief Geometry Shader.
class GeometryShader : public Shader {

    friend class Shader;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// @brief Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    GeometryShader(GraphicsContextPtr& context, const GLuint program, std::string name);

public:
    static GeometryShaderPtr load(GraphicsContextPtr& context, std::string name, const std::string& file)
    {
        return std::static_pointer_cast<GeometryShader>(
            _load(context, std::move(name), Pipeline::Stage::GEOMETRY, file));
    }

    /// @brief Pipeline stage of the Shader.
    virtual Pipeline::Stage stage() const override final { return Pipeline::Stage::GEOMETRY; }
};

// ===================================================================================================================//

/// @brief Fragment Shader.
class FragmentShader : public Shader {

    friend class Shader;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// @brief Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    FragmentShader(GraphicsContextPtr& context, const GLuint program, std::string name);

public:
    static FragmentShaderPtr load(GraphicsContextPtr& context, std::string name, const std::string& file)
    {
        return std::static_pointer_cast<FragmentShader>(
            _load(context, std::move(name), Pipeline::Stage::FRAGMENT, file));
    }

    /// @brief Pipeline stage of the Shader.
    virtual Pipeline::Stage stage() const override final { return Pipeline::Stage::FRAGMENT; }
};

} // namespace notf
