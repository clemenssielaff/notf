#pragma once

#include <vector>

#include "notf/meta/assert.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/shader.hpp"

NOTF_OPEN_NAMESPACE

// shader program =================================================================================================== //

/// Shader Program.
/// Is not managed by the context, as it can be shared across multiple.
class ShaderProgram : public std::enable_shared_from_this<ShaderProgram> {

    friend Accessor<ShaderProgram, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(ShaderProgram);

    /// A Uniform in this ShaderProgram.
    struct Uniform {
        Shader::Stage::Flags stage;
        Shader::Uniform variable;
    };

    using Attribute = Shader::Uniform;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(ShaderProgram);

    /// Value constructor.
    /// @param name                 Human readable name of the Shader.
    /// @param vertex_shader        Vertex shader to use in the Program.
    /// @param tesselation_shader   Tesselation shader to use in the Program.
    /// @param geometry_shader      Geometry shader to use in the Program.
    /// @param fragment_shader      Fragment shader to use in the Program.
    ShaderProgram(std::string name, VertexShaderPtr get_vertex_shader, TesselationShaderPtr get_tesselation_shader,
                  GeometryShaderPtr get_geometry_shader, FragmentShaderPtr get_fragment_shader);

public:
    /// Factory.
    /// @param name                 Human readable name of the Shader.
    /// @param vertex_shader        Vertex shader to use in the Program.
    /// @param tesselation_shader   Tesselation shader to use in the Program.
    /// @param geometry_shader      Geometry shader to use in the Program.
    /// @param fragment_shader      Fragment shader to use in the Program.
    static ShaderProgramPtr
    create(std::string name, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
           GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader);

    static ShaderProgramPtr create(std::string name, VertexShaderPtr vertex_shader, FragmentShaderPtr fragment_shader) {
        return create(std::move(name), std::move(vertex_shader), {}, {}, std::move(fragment_shader));
    }

    static ShaderProgramPtr create(std::string name, VertexShaderPtr vertex_shader,
                                   TesselationShaderPtr tesselation_shader, FragmentShaderPtr fragment_shader) {
        return create(std::move(name), std::move(vertex_shader), std::move(tesselation_shader), {},
                      std::move(fragment_shader));
    }

    /// Destructor.
    ~ShaderProgram();

    /// Checks if the ShaderProgram is valid.
    /// A ShaderProgram should always be valid - the only way to get an invalid one is to remove the GraphicsSystem
    /// while still holding on to shared pointers of a Shader that lived in the removed GraphicsSystem.
    bool is_valid() const;

    /// OpenGL ID of the Program object.
    ShaderProgramId get_id() const { return m_id; }

    /// The name of this Shader.
    const std::string& get_name() const { return m_name; }

    /// Vertex shader attached to this Program.
    const VertexShaderPtr& get_vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this Program.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    const TesselationShaderPtr& get_tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this Program.
    const GeometryShaderPtr& get_geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this Program.
    const FragmentShaderPtr& get_fragment_shader() const { return m_fragment_shader; }

    /// All uniforms of this shader.
    const std::vector<Uniform>& get_uniforms() const { return m_uniforms; }

    /// All attribute variables.
    const std::vector<Attribute>& get_attributes() { return m_attributes; }

    /// Updates the value of a uniform in the shader.
    /// @throws OpenGlError If the uniform cannot be found.
    /// @throws OpenGlError If the value type and the uniform type are not compatible.
    template<typename T>
    void set_uniform(const std::string& name, const T& value) {
        auto& uniform = _get_uniform(name);
        if (uniform.stage & Shader::Stage::Flag::VERTEX) {
            NOTF_ASSERT(m_vertex_shader);
            m_vertex_shader->set_uniform(name, value);
        } else if (uniform.stage & Shader::Stage::Flag::GEOMETRY) {
            NOTF_ASSERT(m_geometry_shader);
            m_geometry_shader->set_uniform(name, value);
        } else if ((uniform.stage & Shader::Stage::Flag::TESS_CONTROL)
                   || (uniform.stage & Shader::Stage::Flag::TESS_EVALUATION)) {
            NOTF_ASSERT(m_tesselation_shader);
            m_tesselation_shader->set_uniform(name, value);
        } else if (uniform.stage & Shader::Stage::Flag::FRAGMENT) {
            NOTF_ASSERT(m_fragment_shader);
            m_fragment_shader->set_uniform(name, value);
        } else {
            NOTF_ASSERT(false);
        }
    }

private:
    /// Returns the uniform with the given name.
    /// @throws OpenGlError If there is no uniform with the given name in this shader.
    Uniform& _get_uniform(const std::string& name);

    /// Deallocates the Program.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The name of this Shader.
    const std::string m_name;

    /// Vertex shader attached to this Program.
    VertexShaderPtr m_vertex_shader;

    /// Tesselation shader attached to this Program.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    TesselationShaderPtr m_tesselation_shader;

    /// Geometry shader attached to this Program.
    GeometryShaderPtr m_geometry_shader;

    /// Fragment shader attached to this Program.
    FragmentShaderPtr m_fragment_shader;

    ///  All uniforms of this shader.
    std::vector<Uniform> m_uniforms;

    /// All attributes of this Shader.
    std::vector<Attribute> m_attributes;

    /// OpenGL ID of the Program object.
    ShaderProgramId m_id = 0;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<ShaderProgram, detail::GraphicsSystem> {
    friend detail::GraphicsSystem;

    /// Deallocates the Program.
    static void deallocate(ShaderProgram& program) { program._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
