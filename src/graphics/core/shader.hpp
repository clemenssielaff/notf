#pragma once

#include <string>
#include <vector>

#include "common/signal.hpp"
#include "graphics/ids.hpp"

NOTF_OPEN_NAMESPACE

// TODO: cache compiled shader binaries next to their text files (like python?)

// ================================================================================================================== //

/// Manages the loading and compilation of an OpenGL shader.
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

    // types -------------------------------------------------------------------------------------------------------- //
public:
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
        enum Flag : unsigned char {
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
    /// @param context  Render Context in which the Shader lives.
    /// @param id       OpenGL Shader program ID.
    /// @param stages   Pipeline stage/s of the Shader.
    /// @param name     Name of this Shader.
    Shader(GraphicsContext& context, const GLuint id, Stage::Flags stages, std::string name);

public:
    NOTF_NO_COPY_OR_ASSIGN(Shader);

    /// Destructor
    virtual ~Shader();

    /// Graphics Context in which the Texture lives.
    GraphicsContext& context() const { return m_graphics_context; }

    /// The OpenGL ID of the Shader program.
    ShaderId id() const { return m_id; }

    /// Checks if the Shader is valid.
    /// A Shader should always be valid - the only way to get an invalid one is to remove the GraphicsContext while
    /// still holding on to shared pointers of a Shader that lived in the removed GraphicsContext.
    bool is_valid() const { return m_id.is_valid(); }

    /// Pipeline stage/s of the Shader.
    Stage::Flags stage() const { return m_stages; }

    /// The name of this Shader.
    const std::string& name() const { return m_name; }

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

    /// Registers the given Shader with its context.
    /// @param shader           Shader to register.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static void _register_with_context(const ShaderPtr& shader);

private:
    /// Returns the uniform with the given name.
    /// @throws runtime_error   If there is no uniform with the given name in this shader.
    const Variable& _uniform(const std::string& name) const;

    /// Deallocates the Shader data and invalidates the Shader.
    void _deallocate();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Graphics Context in which the Texture lives.
    GraphicsContext& m_graphics_context;

    //// ID of the shader program.
    ShaderId m_id;

    /// All stages contained in this Shader.
    const Stage::Flags m_stages;

    /// The name of this Shader.
    const std::string m_name;

    ///  All uniforms of this shader.
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

// ================================================================================================================== //

/// Vertex Shader.
class VertexShader : public Shader {

    // signals ------------------------------------------------------------------------------------------------------ //
public:
    /// Fired when a new Vertex Shader was created.
    /// @param New vertex shader.
    static Signal<VertexShaderPtr> on_shader_created;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param source   Source of the Shader.
    VertexShader(GraphicsContext& context, const GLuint program, std::string name, std::string source);

public:
    /// Factory.
    /// @param context          Render Context in which the Shader lives.
    /// @param name             Human readable name of the Shader.
    /// @param source           Vertex shader source.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static VertexShaderPtr create(GraphicsContext& context, std::string name, const std::string& source,
                                  const Defines& defines = s_no_defines);

    /// Returns the location of the attribute with the given name.
    /// @throws runtime_error   If there is no attribute with the given name in this shader.
    GLuint attribute(const std::string& name) const;

    /// All attribute variables.
    const std::vector<Variable>& attributes() { return m_attributes; }

    /// The vertex shader source code.
    const std::string& source() const { return m_source; }

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

    // signals ------------------------------------------------------------------------------------------------------ //
public:
    /// Fired when a new Tesselation Shader was created.
    /// @param New tesselation shader.
    static Signal<TesselationShaderPtr> on_shader_created;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param context              Render Context in which the Shader lives.
    /// @param program              OpenGL Shader program ID.
    /// @param name                 Human readable name of the Shader.
    /// @param control_source       Tesselation control shader source.
    /// @param evaluation_source    Tesselation evaluation shader source.
    TesselationShader(GraphicsContext& context, const GLuint program, std::string name, std::string control_source,
                      std::string evaluation_source);

public:
    /// Factory.
    /// @param context              Render Context in which the Shader lives.
    /// @param name                 Human readable name of the Shader.
    /// @param control_source       Tesselation control shader source.
    /// @param evaluation_source    Tesselation evaluation shader source.
    /// @param defines              Additional definitions to inject into the shader code.
    /// @throws internal_error      If another Shader with the same ID already exists.
    static TesselationShaderPtr create(GraphicsContext& context, const std::string& name, std::string& control_source,
                                       std::string& evaluation_source, const Defines& defines = s_no_defines);

    /// The teselation control shader source code.
    const std::string& control_source() const { return m_control_source; }

    /// The teselation evaluation shader source code.
    const std::string& evaluation_source() const { return m_evaluation_source; }

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

    // signals ------------------------------------------------------------------------------------------------------ //
public:
    /// Fired when a new Geometry Shader was created.
    /// @param New geometry shader.
    static Signal<GeometryShaderPtr> on_shader_created;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param source   Source of the Shader.
    GeometryShader(GraphicsContext& context, const GLuint program, std::string name, std::string source);

public:
    /// Factory.
    /// @param context          Render Context in which the Shader lives.
    /// @param name             Human readable name of the Shader.
    /// @param source           Geometry shader source.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static GeometryShaderPtr create(GraphicsContext& context, std::string name, const std::string& source,
                                    const Defines& defines = s_no_defines);

    /// The geometry shader source code.
    const std::string& source() const { return m_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Geometry Shader code (including injections).
    const std::string m_source;
};

// ================================================================================================================== //

/// Fragment Shader.
class FragmentShader : public Shader {

    // signals ------------------------------------------------------------------------------------------------------ //
public:
    /// Fired when a new Fragment Shader was created.
    /// @param New fragment shader.
    static Signal<FragmentShaderPtr> on_shader_created;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value Constructor.
    /// @param context  Render Context in which the Shader lives.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param source   Source of the Shader.
    FragmentShader(GraphicsContext& context, const GLuint program, std::string name, std::string source);

public:
    /// Factory.
    /// @param context          Render Context in which the Shader lives.
    /// @param name             Human readable name of the Shader.
    /// @param source           Fragment shader source.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static FragmentShaderPtr create(GraphicsContext& context, std::string name, const std::string& source,
                                    const Defines& defines = s_no_defines);

    /// The fragment shader source code.
    const std::string& source() const { return m_source; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Fragment Shader code (including injections).
    const std::string m_source;
};

NOTF_CLOSE_NAMESPACE
