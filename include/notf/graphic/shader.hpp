#pragma once

#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/fwd.hpp"

// TODO: cache compiled shader binaries next to their text files (like python?)

NOTF_OPEN_NAMESPACE

// shader =========================================================================================================== //

/// Manages the loading and compilation of an OpenGL shader.
///
/// Shader baseclass.
/// Represents a single stage in a ShaderProgram.
/// Technically, OpenGL would call this a "program" containing a single "shader" - but in notf you only have shaders and
/// piplines, so we ignore the nomenclature here.
class Shader : public std::enable_shared_from_this<Shader> {

    friend Accessor<Shader, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Shader);

    struct Stage {
        /// Individual Shader stages.
        enum Flag {
            // implicit zero value for default-initialized Stage
            VERTEX = 1u << 0,          ///< Vertex stage.
            TESS_CONTROL = 1u << 1,    ///< Tesselation control stage.
            TESS_EVALUATION = 1u << 2, ///< Tesselation evaluation stage.
            GEOMETRY = 1u << 3,        ///< Geometry stage.
            FRAGMENT = 1u << 4,        ///< Fragment stage.
            COMPUTE = 1u << 5,         ///< Compute shader (not a stage in the program).
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

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param id       OpenGL Shader program ID.
    /// @param stages   Program stage/s of the Shader.
    /// @param name     Name of this Shader.
    Shader(const GLuint id, Stage::Flags stages, std::string name)
        : m_name(std::move(name)), m_id(id), m_stages(stages) {}

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

    /// Program stage/s of the Shader.
    Stage::Flags get_stage() const { return m_stages; }

    /// The name of this Shader.
    const std::string& get_name() const { return m_name; }

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
    /// Deallocates the Shader data and invalidates the Shader.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The name of this Shader.
    const std::string m_name;

    //// ID of the shader program.
    ShaderId m_id = 0;

    /// All stages contained in this Shader.
    const Stage::Flags m_stages;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<Shader, detail::GraphicsSystem> {
    friend detail::GraphicsSystem;

    /// Deallocates the Shader data and invalidates the Shader.
    static void deallocate(Shader& shader) { shader._deallocate(); }
};

// vertex shader ==================================================================================================== //

/// Vertex Shader.
class VertexShader : public Shader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    VertexShader(const GLuint program, std::string name, std::string string)
        : Shader(program, Stage::VERTEX, std::move(name)), m_source(std::move(string)) {}

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Vertex shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static VertexShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// The vertex shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertex Shader code (including injections).
    const std::string m_source;
};

// tesselation shader =============================================================================================== //

/// Tesselation Shader.
class TesselationShader : public Shader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(TesselationShader);

    /// Value Constructor.
    /// @param program              OpenGL Shader program ID.
    /// @param name                 Human readable name of the Shader.
    /// @param control_string       Tesselation control shader source string.
    /// @param evaluation_string    Tesselation evaluation shader source string.
    TesselationShader(const GLuint program, std::string name, const std::string& control_string,
                      const std::string& evaluation_string)
        : Shader(program, Stage::TESS_CONTROL | Stage::TESS_EVALUATION, std::move(name))
        , m_control_source(std::move(control_string))
        , m_evaluation_source(std::move(evaluation_string)) {}

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

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Teselation control shader code.
    const std::string m_control_source;

    /// Teselation evaluation shader code.
    const std::string m_evaluation_source;
};

// geometry shader ================================================================================================== //

/// Geometry Shader.
class GeometryShader : public Shader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(GeometryShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    GeometryShader(const GLuint program, std::string name, std::string string)
        : Shader(program, Stage::GEOMETRY, std::move(name)), m_source(std::move(string)) {}

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Geometry shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static GeometryShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// The geometry shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Geometry Shader code (including injections).
    const std::string m_source;
};

// fragment shader ================================================================================================== //

/// Fragment Shader.
class FragmentShader : public Shader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(FragmentShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param string   Source string of the Shader.
    FragmentShader(const GLuint program, std::string name, std::string string)
        : Shader(program, Stage::FRAGMENT, std::move(name)), m_source(std::move(string)) {}

public:
    /// Factory.
    /// @param name             Human readable name of the Shader.
    /// @param string           Fragment shader source string.
    /// @param defines          Additional definitions to inject into the shader code.
    /// @throws internal_error  If another Shader with the same ID already exists.
    static FragmentShaderPtr create(std::string name, const std::string& string, const Defines& defines = s_no_defines);

    /// The fragment shader source code.
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Fragment Shader code (including injections).
    const std::string m_source;
};

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for ShaderPtr subclasses.
template<class Lhs, class Rhs>
struct std::common_type<std::shared_ptr<Lhs>, std::shared_ptr<Rhs>> {
    using type = std::enable_if_t< //
        std::conjunction_v<std::is_base_of<::notf::Shader, Lhs>, std::is_base_of<::notf::Shader, Rhs>>,
        ::notf::ShaderPtr>;
};
