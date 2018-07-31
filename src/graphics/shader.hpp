#pragma once

#include <string>
#include <vector>

#include "graphics/ids.hpp"

// TODO: cache compiled shader binaries next to their text files (like python?)

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _Shader;
} // namespace access

// ================================================================================================================== //

/// Manages the loading and compilation of an OpenGL shader.
///
/// Shader baseclass.
/// Represents a single stage in the shading pipeline.
/// Technically, OpenGL would call this a "program" containing a single "shader" - but in notf you only have shaders and
/// piplines, so we ignore the nomenclature here.
///
class Shader : public std::enable_shared_from_this<Shader> {

    friend class access::_Shader<TheGraphicsSystem>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Shader<T>;

    /// Information about a variable (attribute or uniform) of this shader.
    struct Variable {
        /// Location of the variable, used to address the variable in the OpenGL shader.
        GLint location;

        /// Type of the variable.
        /// See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
        GLenum type;

        /// Number of elements in the variable in units of type.
        /// Is always >=1 and only >1 if the variable is an array.
        GLint size;

        /// The name of the variable.
        std::string name;
    };

    struct Stage {
        /// Individual Shader stages.
        enum Flag {
            // implicit zero value for default-initialized Stage
            VERTEX = 1u << 0,          ///< Vertex stage.
            TESS_CONTROL = 1u << 1,    ///< Tesselation control stage.
            TESS_EVALUATION = 1u << 2, ///< Tesselation evaluation stage.
            GEOMETRY = 1u << 3,        ///< Geometry stage.
            FRAGMENT = 1u << 4,        ///< Fragment stage.
            COMPUTE = 1u << 5,         ///< Compute shader (not a stage in the pipeline).
        };

        /// Combination of Shader stages.
        using Flags = std::underlying_type_t<Flag>;
    };

    /// Defines additional defines to inject into the GLSL code.
    using Defines = std::vector<std::pair<std::string, std::string>>;

protected:
    /// Empty Defines used as default argument.
    static const Defines s_no_defines;

    /// Construction arguments.
    struct Args {
        const char* vertex_source = nullptr;
        const char* tess_ctrl_source = nullptr;
        const char* tess_eval_source = nullptr;
        const char* geometry_source = nullptr;
        const char* fragment_source = nullptr;
        const char* compute_source = nullptr;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @param id       OpenGL Shader program ID.
    /// @param stages   Pipeline stage/s of the Shader.
    /// @param name     Name of this Shader.
    Shader(const GLuint id, Stage::Flags stages, std::string name);

public:
    NOTF_NO_COPY_OR_ASSIGN(Shader);

    /// Destructor
    virtual ~Shader();

    /// The OpenGL ID of the Shader program.
    ShaderId get_id() const { return m_id; }

    /// Checks if the Shader is valid.
    /// A Shader should always be valid - the only way to get an invalid one is to remove the GraphicsSystem while
    /// still holding on to shared pointers of a Shader that lived in the removed GraphicsSystem.
    bool is_valid() const { return m_id.is_valid(); }

    /// Pipeline stage/s of the Shader.
    Stage::Flags get_stage() const { return m_stages; }

    /// The name of this Shader.
    const std::string& get_name() const { return m_name; }

    /// All uniforms of this shader.
    const std::vector<Variable>& get_uniforms() const { return m_uniforms; }

    /// Updates the value of a uniform in the shader.
    /// @throws runtime_error   If the uniform cannot be found.
    /// @throws runtime_error   If the value type and the uniform type are not compatible.
    template<typename T>
    void set_uniform(const std::string&, const T&)
    {
        static_assert(always_false_t<T>{}, "No overload for value type in notf::Shader::set_uniform");
    }

#ifdef NOTF_DEBUG
    /// Checks whether the shader can execute in the current OpenGL state.
    /// Is expensive and should only be used for debugging!
    /// @return  True if the validation succeeded.
    bool validate_now() const;
#endif

protected:
    /// Factory.
    /// @param name     Name of this Shader.
    /// @param args     Construction arguments.
    /// @return OpenGL Shader program ID.
    static GLuint _build(const std::string& name, const Args& args);

    /// Registers the given Shader with the GraphicsSystem.
    /// @param shader           Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static void _register_with_system(const ShaderPtr& shader);

private:
    /// Returns the uniform with the given name.
    /// @throws runtime_error   If there is no uniform with the given name in this shader.
    const Variable& _uniform(const std::string& name) const;

    /// Deallocates the Shader data and invalidates the Shader.
    void _deallocate();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The name of this Shader.
    const std::string m_name;

    ///  All uniforms of this shader.
    std::vector<Variable> m_uniforms;

    //// ID of the shader program.
    ShaderId m_id = 0;

    /// All stages contained in this Shader.
    const Stage::Flags m_stages;
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

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Shader<TheGraphicsSystem> {
    friend class notf::TheGraphicsSystem;

    /// Deallocates the Shader data and invalidates the Shader.
    static void deallocate(Shader& shader) { shader._deallocate(); }
};

// ================================================================================================================== //

/// Vertex Shader.
class VertexShader : public Shader {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    VertexShader(const GLuint program, std::string name, std::string string);

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Vertex shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static VertexShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// Returns the location of the attribute with the given name.
    /// @throws runtime_error   If there is no attribute with the given name in this shader.
    GLuint get_attribute(const std::string& name) const;

    /// All attribute variables.
    const std::vector<Variable>& get_attributes() { return m_attributes; }

    /// The vertex shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Vertex Shader code (including injections).
    const std::string m_source;

    /// All attributes of this Shader.
    std::vector<Variable> m_attributes;
};

// ================================================================================================================== //

/// Tesselation Shader.
class TesselationShader : public Shader {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param program              OpenGL Shader program ID.
    /// @param name                 Human readable name of the Shader.
    /// @param control_string       Tesselation control shader source string.
    /// @param evaluation_string    Tesselation evaluation shader source string.
    TesselationShader(const GLuint program, std::string name, const std::string& control_string,
                      const std::string& evaluation_string);

public:
    /// Factory.
    /// @param name                 Human readable name of the Shader.
    /// @param control_string       Tesselation control shader source string.
    /// @param evaluation_string    Tesselation evaluation shader source string.
    /// @param defines              Additional definitions to inject into the shader code.
    /// @throws internal_error      If another Shader with the same ID already exists.
    static TesselationShaderPtr create(const std::string& name, const std::string& control_string,
                                       const std::string& evaluation_string, const Defines& defines = s_no_defines);

    /// The teselation control shader source code.
    const std::string& get_control_source() const { return m_control_source; }

    /// The teselation evaluation shader source code.
    const std::string& get_evaluation_source() const { return m_evaluation_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Teselation control shader code.
    const std::string m_control_source;

    /// Teselation evaluation shader code.
    const std::string m_evaluation_source;
};

// ================================================================================================================== //

/// Geometry Shader.
class GeometryShader : public Shader {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    GeometryShader(const GLuint program, std::string name, std::string string);

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Geometry shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static GeometryShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// The geometry shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Geometry Shader code (including injections).
    const std::string m_source;
};

// ================================================================================================================== //

/// Fragment Shader.
class FragmentShader : public Shader {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    FragmentShader(const GLuint program, std::string name, std::string string);

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Fragment shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static FragmentShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// The fragment shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Fragment Shader code (including injections).
    const std::string m_source;
};

NOTF_CLOSE_NAMESPACE
