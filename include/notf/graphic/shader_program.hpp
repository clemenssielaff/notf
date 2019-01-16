#pragma once

#include <vector>

#include "notf/meta/assert.hpp"
#include "notf/meta/numeric.hpp"

#include "notf/graphic/shader.hpp"

NOTF_OPEN_NAMESPACE

// shader program =================================================================================================== //

/// Shader Program.
/// Is not managed by a single context because it can be shared.
class ShaderProgram : public std::enable_shared_from_this<ShaderProgram> {

    friend Accessor<ShaderProgram, GraphicsContext>;
    friend Accessor<ShaderProgram, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
private:
    /// Information about a variable (attribute or uniform) of this ShaderProgram.
    struct Variable {
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

public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(ShaderProgram);

    // uniform ----------------------------------------------------------------

    /// A Uniform in this ShaderProgram.
    class Uniform {
        friend ShaderProgram;

        // methods --------------------------------------------------------- //
    private:
        Uniform(ShaderProgram& program, GLint location, Shader::Stage::Flags stages, Variable&& variable);

    public:
        /// Updates the value of a uniform in the ShaderProgram.
        /// @throws ValueError  If the value type and the uniform type are not compatible.
        template<typename T>
        void set(const T& value) {
            if (m_stages & Shader::Stage::Flag::VERTEX) {
                m_program._set_uniform(m_program.m_vertex_shader, *this, value);
            } else if (m_stages & Shader::Stage::Flag::GEOMETRY) {
                m_program._set_uniform(m_program.m_geometry_shader, *this, value);
            } else if ((m_stages & Shader::Stage::Flag::TESS_CONTROL)
                       || (m_stages & Shader::Stage::Flag::TESS_EVALUATION)) {
                m_program._set_uniform(m_program.m_tesselation_shader, *this, value);
            } else if (m_stages & Shader::Stage::Flag::FRAGMENT) {
                m_program._set_uniform(m_program.m_fragment_shader, *this, value);
            } else {
                NOTF_ASSERT(false);
            }
        }

        GLint get_location() const { return m_location; }

        GLenum get_type() const { return m_variable.type; }

        const std::string& get_name() const { return m_variable.name; }

        // fields ---------------------------------------------------------- //
    private:
        /// ShaderProgram owning this uniform block.
        const ShaderProgram& m_program;

        /// The location of this uniform variable within the default uniform block of a ShaderProgram.
        GLint m_location = -1;

        /// Stage of the graphics pipeline that this Uniform is referenced.
        Shader::Stage::Flags m_stages;

        /// Actual variable of the Uniform.
        Variable m_variable;
    };

    // uniform block ----------------------------------------------------------

    /// An UniformBlock in this ShaderProgram.
    struct UniformBlock {
        friend ShaderProgram;

        // methods --------------------------------------------------------- //
    private:
        UniformBlock(const ShaderProgram& program, const GLuint index);

    public:
        GLuint get_index() const { return m_index; }

        Shader::Stage::Flags get_stages() const { return m_stages; }

        GLuint get_data_size() const { return m_data_size; }

        const std::string& get_name() const { return m_name; }

        // TODO: I don't actually think I need all of this "storing OpenGL state to avoid unnecessary calls"
        //       because binding/unbinding should happen exactly once, when a new ShaderProgram is bound
        AnyUniformBufferPtr get_bound_buffer() const { return m_buffer.lock(); }

    private:
        /// Binds this Uniform
        /// @param context  GraphicsContext managing the binding.
        /// @param buffer   UniformBuffer to bind to.
        /// @param slot     UniformBuffer slot to bind at.
        /// @param offset   Offset of the UniformBuffer (in blocks) to bind.
        void _bind_to(GraphicsContext& context, const AnyUniformBufferPtr& buffer, const GLuint slot,
                      const size_t offset);

        /// Unbind from the UniformBuffer, does nothing if the block isn't bound.
        /// @param context  GraphicsContext managing the binding.
        void _unbind(GraphicsContext& context);

        // fields ---------------------------------------------------------- //
    private:
        /// ShaderProgram owning this uniform block.
        const ShaderProgram& m_program;

        /// Name of the uniform block.
        const std::string m_name;

        /// Index of the uniform block in the Shader.
        const GLuint m_index;

        /// Stages that the uniform block appears in (vertex and/or fragment).
        const Shader::Stage::Flags m_stages;

        /// Size of the UniformBlock in bytes.
        const GLuint m_data_size;

        /// Uniform buffer slot to which the block is currently bound.
        GLuint m_slot = max_value<GLuint>(); // invalid default

        /// Uniform buffer to which this UniformBlock is bound (can be empty).
        AnyUniformBufferWeakPtr m_buffer;

        /// Index at which the UniformBuffer is bound to the slot.
        size_t m_offset = max_value<size_t>(); // invalid default

        /// All uniforms in the block.
        std::vector<Variable> m_variables;
    };

    static_assert(sizeof(UniformBlock) == 96);

    // attribute --------------------------------------------------------------

    /// An Attribute in this ShaderProgram.
    using Attribute = Variable;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(ShaderProgram);

    /// Value constructor.
    /// @param name         Human readable name of the ShaderProgram.
    /// @param vert_shader  Vertex shader to use in the ShaderProgram.
    /// @param tess_shader  Tesselation shader to use in the ShaderProgram.
    /// @param geo_shader   Geometry shader to use in the ShaderProgram.
    /// @param frag_shader  Fragment shader to use in the ShaderProgram.
    ShaderProgram(std::string name,                 //
                  VertexShaderPtr vert_shader,      //
                  TesselationShaderPtr tess_shader, //
                  GeometryShaderPtr geo_shader,     //
                  FragmentShaderPtr frag_shader);

public:
    /// Factory.
    /// @param name         Human readable name of the ShaderProgram.
    /// @param vert_shader  Vertex shader to use in the ShaderProgram.
    /// @param tess_shader  Tesselation shader to use in the ShaderProgram.
    /// @param geo_shader   Geometry shader to use in the ShaderProgram.
    /// @param frag_shader  Fragment shader to use in the ShaderProgram.
    static ShaderProgramPtr create(std::string name,                  //
                                   VertexShaderPtr vert_shader,       //
                                   TesselationShaderPtr tess_shader,  //
                                   GeometryShaderPtr geometry_shader, //
                                   FragmentShaderPtr frag_shader);

    static ShaderProgramPtr create(std::string name,            //
                                   VertexShaderPtr vert_shader, //
                                   FragmentShaderPtr frag_shader) {
        return create(std::move(name), std::move(vert_shader), {}, {}, std::move(frag_shader));
    }

    static ShaderProgramPtr create(std::string name,                 //
                                   VertexShaderPtr vert_shader,      //
                                   TesselationShaderPtr tess_shader, //
                                   FragmentShaderPtr frag_shader) {
        return create(std::move(name), std::move(vert_shader), std::move(tess_shader), {}, std::move(frag_shader));
    }

    /// Destructor.
    ~ShaderProgram();

    /// Checks if the ShaderProgram is valid.
    /// A ShaderProgram should always be valid - the only way to get an invalid one is to remove the GraphicsSystem
    /// while still holding on to shared pointers of a ShaderProgram that lived in the removed GraphicsSystem.
    bool is_valid() const;

    /// OpenGL ID of the ShaderProgram object.
    ShaderProgramId get_id() const { return m_id; }

    /// The name of this ShaderProgram.
    const std::string& get_name() const { return m_name; }

    /// Vertex shader attached to this ShaderProgram.
    const VertexShaderPtr& get_vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this ShaderProgram.
    /// The tesselation stage actually contains two ShaderSources (tesselation control and -evaluation).
    const TesselationShaderPtr& get_tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this ShaderProgram.
    const GeometryShaderPtr& get_geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this ShaderProgram.
    const FragmentShaderPtr& get_fragment_shader() const { return m_fragment_shader; }

    /// @{
    /// Returns the uniform with the given name.
    /// @throws NameError   If there is no uniform with the given name in this ShaderProgram.
    const Uniform& get_uniform(const std::string& name) const;
    Uniform& get_uniform(const std::string& name) { return NOTF_FORWARD_CONST_AS_MUTABLE(get_uniform(name)); }
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

private:
    /// Create the ShaderProgram pipeline.
    /// @throws OpenGLError If the program creation failed.
    void _link_program();

    /// Find all uniform blocks in a given Shader.
    void _find_uniform_blocks(const ShaderPtr& shader);

    /// Find all uniforms in a given Shader.
    void _find_uniforms(const ShaderPtr& shader);

    /// Find all atributes in the ShaderProgram.
    void _find_attributes();

    /// Is called when the Program is bound to the GraphicsContext.
    void _activate(GraphicsContext& context);

    /// Is called when the Program is unbound from the GraphicsContext.
    void _deactivate();

    /// Updates the value of a uniform in the ShaderProgram.
    /// @param uniform  Uniform to change.
    /// @param value    Value to update.
    /// @throws ValueError  If the value type and the uniform type are not compatible.
    template<typename T>
    void _set_uniform(const ShaderPtr& shader, const Uniform& uniform, const T& value);

    /// Deallocates the ShaderProgram.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
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
    std::vector<Attribute> m_attributes; // TODO: ShaderProgram VAO?

    /// ID of the OpenGL object.
    ShaderProgramId m_id = 0;
};

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const int&);

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const unsigned int&);

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const float&);

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const V2f&);

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const V4f&);

template<>
void ShaderProgram::_set_uniform(const ShaderPtr&, const Uniform&, const M4f&);

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<ShaderProgram, detail::GraphicsSystem> {
    friend detail::GraphicsSystem;

    /// Deallocates the Program.
    static void deallocate(ShaderProgram& program) { program._deallocate(); }
};

template<>
class Accessor<ShaderProgram, GraphicsContext> {
    friend GraphicsContext;

    /// Is called when the Program is bound to the GraphicsContext.
    static void activate(ShaderProgram& program) { program._activate(); }

    /// Is called when the Program is unbound from the GraphicsContext.
    static void deactivate(ShaderProgram& program) { program._deactivate(); }
};

// CONTINUE HERE:
// This is all backwards - The ShaderProgram (much like a Shader) should be something you set up once and then it's
// basically immutable. It might make sense to change texture bindings etc. but even that can be accompilished by
// simply having two ShaderPrograms with different Textures? ... but that would mean two actual programs on the GPU.
// Actually ... maybe Textures and UniformBindings etc. aren't the jurisdiction of the ShaderProgram after all.
// Instead, they should be part of a `GraphicsSetup`. A `Setup` is ingested by the Context and there you go.


NOTF_CLOSE_NAMESPACE
