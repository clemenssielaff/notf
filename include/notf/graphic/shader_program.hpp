#pragma once

#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// shader program =================================================================================================== //

/// Shader Program.
/// Is not managed by the context, but keeps their Shaders alive.
class ShaderProgram : public std::enable_shared_from_this<ShaderProgram> {

    friend Accessor<ShaderProgram, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(ShaderProgram);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(ShaderProgram);

    /// Value constructor.
    /// @param vertex_shader        Vertex shader to use in the Program.
    /// @param tesselation_shader   Tesselation shader to use in the Program.
    /// @param geometry_shader      Geometry shader to use in the Program.
    /// @param fragment_shader      Fragment shader to use in the Program.
    ShaderProgram(VertexShaderPtr get_vertex_shader, TesselationShaderPtr get_tesselation_shader,
                  GeometryShaderPtr get_geometry_shader, FragmentShaderPtr get_fragment_shader);

public:
    /// Factory.
    /// @param vertex_shader        Vertex shader to use in the Program.
    /// @param tesselation_shader   Tesselation shader to use in the Program.
    /// @param geometry_shader      Geometry shader to use in the Program.
    /// @param fragment_shader      Fragment shader to use in the Program.
    static ShaderProgramPtr create(VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
                                   GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader);

    static ShaderProgramPtr create(VertexShaderPtr vertex_shader, FragmentShaderPtr fragment_shader) {
        return create(std::move(vertex_shader), {}, {}, std::move(fragment_shader));
    }

    static ShaderProgramPtr
    create(VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader, FragmentShaderPtr fragment_shader) {
        return create(std::move(vertex_shader), std::move(tesselation_shader), {}, std::move(fragment_shader));
    }

    /// Destructor.
    ~ShaderProgram();

    /// OpenGL ID of the Program object.
    ShaderProgramId get_id() const { return m_id; }

    /// Vertex shader attached to this Program.
    const VertexShaderPtr& get_vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this Program.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    const TesselationShaderPtr& get_tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this Program.
    const GeometryShaderPtr& get_geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this Program.
    const FragmentShaderPtr& get_fragment_shader() const { return m_fragment_shader; }

private:
    /// Deallocates the Program.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertex shader attached to this Program.
    VertexShaderPtr m_vertex_shader;

    /// Tesselation shader attached to this Program.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    TesselationShaderPtr m_tesselation_shader;

    /// Geometry shader attached to this Program.
    GeometryShaderPtr m_geometry_shader;

    /// Fragment shader attached to this Program.
    FragmentShaderPtr m_fragment_shader;

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
