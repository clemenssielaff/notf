#pragma once

#include "notf/graphic/shader.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// shader variable ================================================================================================== //

/// Information about a variable (attribute or uniform) in a ShaderProgram.
struct ShaderVariable {
    /// Index of the uniform variable in the Shader.
    GLuint index;

    /// Type of the variable.
    /// See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
    GLenum type = 0;

    /// Number of elements in the variable in units of type.
    /// Is always >=1 and only >1 if the variable is an array.
    GLint size = 0;

    /// The name of the variable.
    std::string name;
};

// uniform ========================================================================================================== //

/// A Uniform in this ShaderProgram.
class Uniform {

    friend ShaderProgram;

    // methods --------------------------------------------------------------------------------- //
private:
    /// Constructor.
    /// @param program  ShaderProgram managing this Uniform.
    /// @param location The location of this uniform variable within the default uniform block of a ShaderProgram.
    /// @param stages   Stage/s of the graphics pipeline that this Uniform is referenced.
    /// @param variable Actual variable of the Uniform.
    Uniform(ShaderProgram& program, GLint location, AnyShader::Stage::Flags stages, ShaderVariable variable)
        : m_program(program), m_location(location), m_stages(stages), m_variable(std::move(variable)) {}

public:
    /// The name of the variable.
    const std::string& get_name() const { return m_variable.name; }

    /// The location of this uniform variable within the default uniform block of a ShaderProgram.
    GLint get_location() const { return m_location; }

    /// Type of the variable.
    /// See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
    GLenum get_type() const { return m_variable.type; }

    /// Stage/s of the graphics pipeline that this Uniform is referenced.
    AnyShader::Stage::Flags get_states() const { return m_stages; }

    /// Updates the value of a uniform in the ShaderProgram.
    /// @throws ValueError  If the value type and the uniform type are not compatible.
    template<class T>
    void set(const T& value);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// ShaderProgram owning this uniform block.
    ShaderProgram& m_program;

    /// The location of this uniform variable within the default uniform block of a ShaderProgram.
    GLint m_location;

    /// Stage of the graphics pipeline that this Uniform is referenced.
    AnyShader::Stage::Flags m_stages;

    /// Actual variable of the Uniform.
    ShaderVariable m_variable;
};

// uniform block ==================================================================================================== //

/// An UniformBlock in this ShaderProgram.
class UniformBlock {

    friend ShaderProgram;

    // methods --------------------------------------------------------- //
private:
    /// Constructor.
    /// @param program  ShaderProgram managing this UniformBlock.
    /// @param index    Index of the UniformBlock in the Shader.
    UniformBlock(ShaderProgram& program, const GLuint index);

public:
    /// Name of the UniformBlock.
    const std::string& get_name() const { return m_name; }

    /// ShaderProgram owning this UniformBlock.
    const ShaderProgram& get_program() const { return m_program; }

    /// Index of the UniformBlock in the Shader.
    GLuint get_index() const { return m_index; }

    /// Stages that the UniformBlock appears in (vertex and/or fragment).
    AnyShader::Stage::Flags get_stages() const { return m_stages; }

    /// Size of the UniformBlock in bytes.
    GLuint get_data_size() const { return m_data_size; }

    /// All variables in the UniformBlock.
    const std::vector<ShaderVariable>& get_variables() { return m_variables; }

    // fields ---------------------------------------------------------- //
private:
    /// ShaderProgram owning this UniformBlock.
    ShaderProgram& m_program;

    /// Name of the UniformBlock.
    const std::string m_name;

    /// Index of the UniformBlock in the Shader.
    const GLuint m_index;

    /// Stages that the UniformBlock appears in (vertex and/or fragment).
    const AnyShader::Stage::Flags m_stages;

    /// Size of the UniformBlock in bytes.
    const GLuint m_data_size;
    // TODO: when binding uniform block to -buffer, make sure that the data size is compatible

    /// All variables in the UniformBlock.
    std::vector<ShaderVariable> m_variables;
};

} // namespace detail

// shader program =================================================================================================== //

/// Shader Program.
///
/// ShaderPrograms are owned by a GraphicContext, and managed by the user through `shared_ptr`s. The
/// ShaderProgram is deallocated when the last `shared_ptr` goes out of scope or the associated GraphicsContext is
/// deleted, whatever happens first. Trying to modify a `shared_ptr` to a deallocated ShaderProgram will throw an
/// exception.
class ShaderProgram : public std::enable_shared_from_this<ShaderProgram> {

    friend detail::Uniform;
    friend Accessor<ShaderProgram, GraphicsContext>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(ShaderProgram);

    /// An Uniform of this ShaderProgram.
    using Uniform = detail::Uniform;

    /// An UniformBlock of this ShaderProgram.
    using UniformBlock = detail::UniformBlock;

    /// An Attribute in this ShaderProgram.
    using Attribute = detail::ShaderVariable;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(ShaderProgram);

    /// Value constructor.
    /// @param context      GraphicsContext managing this ShaderProgram.
    /// @param name         Human readable name of the ShaderProgram.
    /// @param vert_shader  Vertex shader to use in the ShaderProgram.
    /// @param tess_shader  Tesselation shader to use in the ShaderProgram.
    /// @param geo_shader   Geometry shader to use in the ShaderProgram.
    /// @param frag_shader  Fragment shader to use in the ShaderProgram.
    ShaderProgram(GraphicsContext& context,         //
                  std::string name,                 //
                  VertexShaderPtr vert_shader,      //
                  TesselationShaderPtr tess_shader, //
                  GeometryShaderPtr geo_shader,     //
                  FragmentShaderPtr frag_shader);

public:
    /// @{
    /// Factories.
    /// @param context      GraphicsContext managing this ShaderProgram.
    /// @param name         Human readable name of the ShaderProgram.
    /// @param vert_shader  Vertex shader to use in the ShaderProgram.
    /// @param tess_shader  Tesselation shader to use in the ShaderProgram.
    /// @param geo_shader   Geometry shader to use in the ShaderProgram.
    /// @param frag_shader  Fragment shader to use in the ShaderProgram.
    static ShaderProgramPtr create(GraphicsContext& context,          //
                                   std::string name,                  //
                                   VertexShaderPtr vert_shader,       //
                                   TesselationShaderPtr tess_shader,  //
                                   GeometryShaderPtr geometry_shader, //
                                   FragmentShaderPtr frag_shader);

    static ShaderProgramPtr create(GraphicsContext& context,    //
                                   std::string name,            //
                                   VertexShaderPtr vert_shader, //
                                   FragmentShaderPtr frag_shader) {
        return create(context, std::move(name), std::move(vert_shader), {}, {}, std::move(frag_shader));
    }

    static ShaderProgramPtr create(GraphicsContext& context,         //
                                   std::string name,                 //
                                   VertexShaderPtr vert_shader,      //
                                   TesselationShaderPtr tess_shader, //
                                   FragmentShaderPtr frag_shader) {
        return create(context, std::move(name), std::move(vert_shader), std::move(tess_shader), {},
                      std::move(frag_shader));
    }
    /// @}

    /// Destructor.
    ~ShaderProgram();

    /// Checks if the ShaderProgram is valid.
    /// If the GraphicsContext managing this ShaderProgram is destroyed while there is still one or more`shared_ptr`s to
    /// this ShaderProgram, the ShaderProgram will become invalid and all attempts of modification will throw an
    /// exception.
    bool is_valid() const { return m_id.is_valid(); }

    /// GraphicsContext managing this ShaderProgram.
    GraphicsContext& get_context() const { return m_context; }

    /// OpenGL ID of the ShaderProgram object.
    ShaderProgramId get_id() const { return m_id; }

    /// The name of this ShaderProgram.
    const std::string& get_name() const { return m_name; }

    /// Vertex shader attached to this ShaderProgram, can be empty.
    const VertexShaderPtr& get_vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this ShaderProgram, can be empty.
    /// The tesselation stage actually contains two ShaderSources (tesselation control and -evaluation).
    const TesselationShaderPtr& get_tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this ShaderProgram, can be empty.
    const GeometryShaderPtr& get_geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this ShaderProgram, can be empty.
    const FragmentShaderPtr& get_fragment_shader() const { return m_fragment_shader; }

    /// @{
    /// Returns the uniform with the given name.
    /// @param name         Name of the requested Uniform.
    /// @throws NameError   If there is no uniform with the given name in this ShaderProgram.
    const Uniform& get_uniform(const std::string& name) const;
    Uniform& get_uniform(const std::string& name) { return NOTF_FORWARD_CONST_AS_MUTABLE(get_uniform(name)); }
    /// @}

    /// @{
    /// Returns the uniform at the given location.
    /// @param location     Location of the requested Uniform.
    /// @throws IndexError  If there is no Uniform with the given location in this ShaderProgram.
    const Uniform& get_uniform(const GLint location) const;
    Uniform& get_uniform(const GLint location) { return NOTF_FORWARD_CONST_AS_MUTABLE(get_uniform(location)); }
    /// @}

    /// @{
    /// Uniform block with the given name.
    /// @param name         Name of the requested UniformBlock.
    /// @throws NameError   If this ShaderProgram does not have a UniformBlock with the requested name.
    const UniformBlock& get_uniform_block(const std::string& name) const;
    UniformBlock& get_uniform_block(const std::string& name) {
        return NOTF_FORWARD_CONST_AS_MUTABLE(get_uniform_block(name));
    }
    /// @}

    /// @{
    /// Uniform block with the given index.
    /// @param name         Index of the requested UniformBlock.
    /// @throws IndexError  If this ShaderProgram does not have a UniformBlock with the requested index.
    const UniformBlock& get_uniform_block(const GLuint index) const;
    UniformBlock& get_uniform_block(const GLuint index) {
        return NOTF_FORWARD_CONST_AS_MUTABLE(get_uniform_block(index));
    }
    /// @}

private:
    /// Create the ShaderProgram pipeline.
    /// @throws OpenGLError If the program creation failed.
    void _link_program();

    /// Find all uniform blocks in a given Shader.
    void _find_uniform_blocks(const AnyShaderPtr& shader);

    /// Find all uniforms in a given Shader.
    void _find_uniforms(const AnyShaderPtr& shader);

    /// Find all atributes in the ShaderProgram.
    void _find_attributes();

    /// Updates the value of a uniform in the ShaderProgram.
    /// @param shader_id    OpenGL ID of the Shader object that the Uniform lives on.
    /// @param uniform      Uniform to change.
    /// @param value        Value to update.
    /// @throws ValueError  If the value type and the uniform type are not compatible.
    template<typename T>
    void _set_uniform(const GLuint shader_id, const Uniform& uniform, const T& value);

    /// Deallocates the ShaderProgram.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// GraphicsContext managing this ShaderProgram.
    GraphicsContext& m_context;

    /// Human-readable name of this ShaderProgram.
    const std::string m_name;

    /// The attached vertex shader (can be empty).
    VertexShaderPtr m_vertex_shader;

    /// The attached tesselation shader (can be empty).
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    TesselationShaderPtr m_tesselation_shader;

    /// The attached geometry shader (can be empty).
    GeometryShaderPtr m_geometry_shader;

    /// The attached fragment shader (can be empty).
    FragmentShaderPtr m_fragment_shader;

    ///  All uniforms.
    std::vector<Uniform> m_uniforms;

    /// All uniform blocks.
    std::vector<UniformBlock> m_uniform_blocks;

    /// All attributes.
    std::vector<Attribute> m_attributes;
    // TODO: when binding VAOs and ShaderPrograms, check if the layout matches

    /// ID of the OpenGL object.
    ShaderProgramId m_id = ShaderProgramId::invalid();
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<ShaderProgram, GraphicsContext> {
    friend GraphicsContext;

    /// GraphicsContext managing the given ShaderProgram.
    static const GraphicsContext* get_graphics_context(ShaderProgram& program) { return &program.m_context; }

    /// Deallocates the Program.
    static void deallocate(ShaderProgram& program) { program._deallocate(); }
};

// template definitions --------------------------------------------------------------------------------------------- //

template<class T>
void detail::Uniform::set(const T& value) {
    // find the targeted shader
    AnyShaderPtr target_shader;
    const char* stage_name = nullptr;
    if (m_stages & AnyShader::Stage::Flag::VERTEX) {
        stage_name = AnyShader::Stage::get_name(AnyShader::Stage::VERTEX);
        target_shader = m_program.get_vertex_shader();
    } else if (m_stages & AnyShader::Stage::Flag::GEOMETRY) {
        stage_name = AnyShader::Stage::get_name(AnyShader::Stage::GEOMETRY);
        target_shader = m_program.get_geometry_shader();
    } else if ((m_stages & AnyShader::Stage::Flag::TESS_CONTROL)
               || (m_stages & AnyShader::Stage::Flag::TESS_EVALUATION)) {
        stage_name = "tesselation";
        target_shader = m_program.get_tesselation_shader();
    } else if (m_stages & AnyShader::Stage::Flag::FRAGMENT) {
        stage_name = AnyShader::Stage::get_name(AnyShader::Stage::FRAGMENT);
        target_shader = m_program.get_fragment_shader();
    }
    if (!target_shader) {
        NOTF_THROW(InternalError, "Could not find \"{}\" Shader referenced by uniform \"{}\" in ShaderProgram \"{}\"",
                   stage_name, get_name(), m_program.get_name());
    }

    // update the uniform
    m_program._set_uniform(target_shader->get_id().get_value(), *this, value);
}

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const int&);

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const unsigned int&);

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const float&);

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const V2f&);

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const V4f&);

template<>
void ShaderProgram::_set_uniform(const GLuint, const Uniform&, const M4f&);

NOTF_CLOSE_NAMESPACE
