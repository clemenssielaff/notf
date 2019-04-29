#pragma once

#include <vector>

#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/opengl.hpp"

// TODO: cache compiled shader binaries next to their text files (like python?)

NOTF_OPEN_NAMESPACE

// any shader ======================================================================================================= //

/// Manages the loading and compilation of a single OpenGL shader.
///
/// We use `ShaderProgram` as a synonym for an OpenGL "program pipeline", which are multiple "shader programs" (which we
/// simply call "Shader") linked into a single entity.
///
/// Shaders are shared among all GraphicContexts and managed through `shared_ptr`s. When TheGraphicSystem goes out of
/// scope, all Shaders will be deallocated. Trying to modify a deallocated Shader will throw an exception.
class AnyShader {

    friend Accessor<AnyShader, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(AnyShader);

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

        /// Human-readable name of the given Stage.
        static const char* get_name(Flag stage) {
            switch (stage) {
            case VERTEX: return "vertex";
            case TESS_CONTROL: return "tesselation-control";
            case TESS_EVALUATION: return "tesselation-evaluation";
            case GEOMETRY: return "geometry";
            case FRAGMENT: return "fragment";
            case COMPUTE: return "compute";
            }
        }

        /// Combination of Shader stages.
        using Flags = std::underlying_type_t<Flag>;
    };

    /// Additional definitions injected into the GLSL Shader code in the form `#define name value;`
    struct Definition {
        /// Name of the definition.
        std::string name;

        /// Value of the definition.
        std::string value;

        /// Equality operator.
        /// @param other    Other definition to compare to.
        bool operator==(const Definition& other) const { return name == other.name && value == other.value; }
    };
    using Definitions = std::vector<Definition>;

protected:
    /// Empty Defines used as default argument.
    static inline const Definitions s_no_definitions = {};

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
    /// @param id           OpenGL Shader program ID.
    /// @param stages       Program stage/s of the Shader.
    /// @param name         Name of this Shader.
    /// @param definitions  User-given definitions that were injected in the shader code.
    AnyShader(const GLuint id, Stage::Flags stages, std::string name, Definitions definitions)
        : m_name(std::move(name)), m_id(id), m_stages(stages), m_definitions(std::move(definitions)) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyShader);

    /// Destructor
    virtual ~AnyShader();

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

    /// All user-given definitions that were injected in the shader.
    const Definitions& get_definitions() const { return m_definitions; }

    /// Injects a notf header, including system-dependent pragmas and user definitions into the source string.
    /// @param source       GLSL source string to modify.
    /// @param definitions  Definitions to inject.
    /// @returns A new source string containing the custom header.
    static std::string inject_header(const std::string& source, const Definitions& definitions);

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
    /// @throws NotUniquError   If another Shader with the same ID already exists.
    static void _register_with_system(const AnyShaderPtr& shader);

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

    /// All user-given definitions that were injected in the shader.
    const Definitions m_definitions;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<AnyShader, detail::GraphicsSystem> {
    friend detail::GraphicsSystem;

    /// Deallocates the Shader data and invalidates the Shader.
    static void deallocate(AnyShader& shader) { shader._deallocate(); }
};

// vertex shader ==================================================================================================== //

/// Vertex Shader.
class VertexShader : public AnyShader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexShader);

    /// Value Constructor.
    /// @param program      OpenGL Shader program ID.
    /// @param name         Human readable name of the Shader.
    /// @param source       Source string of the Shader as given by the user.
    /// @param definitions  User-given definitions that were injected in the shader code.
    VertexShader(const GLuint program, std::string name, std::string source, Definitions definitions)
        : AnyShader(program, Stage::VERTEX, std::move(name), std::move(definitions)), m_source(std::move(source)) {}

public:
    /// Destructor.
    ~VertexShader();

    /// Factory.
    /// @param name         Human readable name of the Shader.
    /// @param source       Vertex shader source string.
    /// @param definitions  Additional definitions to inject into the shader code.
    /// @throws OpenGLError If the OpenGL shader program could not be created or linked.
    static VertexShaderPtr create(std::string name, std::string source,
                                  const Definitions& definitions = s_no_definitions);

    /// The vertex shader source code as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_source(), shader.get_definitions())
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertex Shader code as given by the user.
    const std::string m_source;
};

// tesselation shader =============================================================================================== //

/// Tesselation Shader.
class TesselationShader : public AnyShader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(TesselationShader);

    /// Value Constructor.
    /// @param program              OpenGL Shader program ID.
    /// @param name                 Human readable name of the Shader.
    /// @param control_source       Tesselation control shader source string.
    /// @param evaluation_source    Tesselation evaluation shader source string.
    /// @param definitions  User-given definitions that were injected in the shader code.
    TesselationShader(const GLuint program, std::string name, std::string control_source, std::string evaluation_source,
                      Definitions definitions)
        : AnyShader(program, Stage::TESS_CONTROL | Stage::TESS_EVALUATION, std::move(name), std::move(definitions))
        , m_control_source(std::move(control_source))
        , m_evaluation_source(std::move(evaluation_source)) {}

public:
    /// Destructor.
    ~TesselationShader();

    /// Factory.
    /// @param name                 Human readable name of the Shader.
    /// @param control_source       Tesselation control shader source string.
    /// @param evaluation_source    Tesselation evaluation shader source string.
    /// @param definitions          Additional definitions to inject into the shader code.
    /// @throws OpenGLError         If the OpenGL shader program could not be created or linked.
    static TesselationShaderPtr create(std::string name, std::string control_source, std::string evaluation_source,
                                       const Definitions& definitions = s_no_definitions);

    /// The teselation control shader source code as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_control_source(), shader.get_definitions())
    const std::string& get_control_source() const { return m_control_source; }

    /// The teselation evaluation shader source code as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_evaluation_source(), shader.get_definitions())
    const std::string& get_evaluation_source() const { return m_evaluation_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Teselation control shader code as given by the user.
    const std::string m_control_source;

    /// Teselation evaluation shader code as given by the user.
    const std::string m_evaluation_source;
};

// geometry shader ================================================================================================== //

/// Geometry Shader.
class GeometryShader : public AnyShader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(GeometryShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param source   Source string of the Shader.
    /// @param definitions  User-given definitions that were injected in the shader code.
    GeometryShader(const GLuint program, std::string name, std::string source, Definitions definitions)
        : AnyShader(program, Stage::GEOMETRY, std::move(name), std::move(definitions)), m_source(std::move(source)) {}

public:
    /// Destructor.
    ~GeometryShader();

    /// Factory.
    /// @param name         Human readable name of the Shader.
    /// @param source       Geometry shader source string.
    /// @param definitions  Additional definitions to inject into the shader code.
    /// @throws OpenGLError If the OpenGL shader program could not be created or linked.
    static GeometryShaderPtr create(std::string name, std::string source,
                                    const Definitions& definitions = s_no_definitions);

    /// The geometry shader source code as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_source(), shader.get_definitions())
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Geometry Shader code as given by the user.
    const std::string m_source;
};

// fragment shader ================================================================================================== //

/// Fragment Shader.
class FragmentShader : public AnyShader {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(FragmentShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param source   Source string of the Shader.
    /// @param definitions  User-given definitions that were injected in the shader code.
    FragmentShader(const GLuint program, std::string name, std::string source, Definitions definitions)
        : AnyShader(program, Stage::FRAGMENT, std::move(name), std::move(definitions)), m_source(std::move(source)) {}

public:
    /// Destructor.
    ~FragmentShader();

    /// Factory.
    /// @param name         Human readable name of the Shader.
    /// @param source       Fragment shader source string.
    /// @param definitions  Additional definitions to inject into the shader code.
    /// @throws OpenGLError If the OpenGL shader program could not be created or linked.
    static FragmentShaderPtr create(std::string name, std::string source,
                                    const Definitions& definitions = s_no_definitions);

    /// The fragment shader source code as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_source(), shader.get_definitions())
    const std::string& get_source() const { return m_source; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Fragment Shader code as given by the user.
    const std::string m_source;
};

// multi stage shader =============================================================================================== //

/// Multi-Stage Shader.
class MultiStageShader : public AnyShader {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Source code for all stages, empty if not used.
    struct Sources {
        std::string vertex;
        std::string tesselation_control;
        std::string tesselation_evaluation;
        std::string geometry;
        std::string fragment;
        std::string compute;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(MultiStageShader);

    /// Value Constructor.
    /// @param program  OpenGL Shader program ID.
    /// @param name     Human readable name of the Shader.
    /// @param sources  Sources of all stages used in the Shader.
    /// @param stages   Stages that are used by this Shader.
    /// @param definitions  User-given definitions that were injected in the shader code.
    MultiStageShader(const GLuint program, std::string name, Sources sources, Stage::Flags stages,
                     Definitions definitions)
        : AnyShader(program, stages, std::move(name), std::move(definitions)), m_sources(std::move(sources)) {}

public:
    /// Destructor.
    ~MultiStageShader();

    /// Factory.
    /// @param name         Human readable name of the Shader.
    /// @param sources      Sources for all stages of the Shader.
    /// @param definitions  Additional definitions to inject into the shader code.
    /// @throws OpenGLError If the OpenGL shader program could not be created or linked.
    static MultiStageShaderPtr create(std::string name, Sources sources,
                                      const Definitions& definitions = s_no_definitions);

    /// Various shader stage sources as passed by the user.
    /// To inspect the source as it passed to OpenGL, call:
    ///     AnyShader::inject_header(shader.get_sources().<STAGE>, shader.get_definitions())
    const Sources& get_sources() const { return m_sources; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Various shader stage sources as given by the user.
    const Sources m_sources;
};

NOTF_CLOSE_NAMESPACE
