#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// vertex object ==================================================================================================== //

class VertexObject {

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexObject);

    /// Constructor.
    /// @param name             Human-readable name of this VertexObject.
    /// @param vertex_buffer    OpenGL buffer storing the vertices.
    /// @param index_buffer     OpenGL buffer storing the indicies.
    /// @throws ValueError      If any of the given buffers are null.
    /// @throws OpenGLError     If the VAO could not be allocated.
    VertexObject(std::string name, AnyVertexBufferPtr vertex_buffer, AnyIndexBufferPtr index_buffer);

public:
    NOTF_NO_COPY_OR_ASSIGN(VertexObject);

    /// Factory.
    /// @param name             Human-readable name of this VertexObject.
    /// @param vertex_buffer    OpenGL buffer storing the vertices.
    /// @param index_buffer     OpenGL buffer storing the indicies.
    /// @throws ValueError      If any of the given buffers are null.
    /// @throws OpenGLError     If the VAO could not be allocated.
    static VertexObjectPtr create(std::string name, AnyVertexBufferPtr vertex_buffer, AnyIndexBufferPtr index_buffer) {
        return _create_shared(std::move(name), std::move(vertex_buffer), std::move(index_buffer));
    }

    /// Destructor.
    ~VertexObject();

    /// Human-readable name of this VertexObject.
    const std::string& get_name() const { return m_name; }

    /// Id of the OpenGL VAO.
    VertexObjectId get_id() const { return m_id; }

    /// @{
    /// OpenGL buffer storing the vertices.
    AnyVertexBufferPtr& get_vertices() { return m_vertex_buffer; }
    const AnyVertexBufferPtr& get_vertices() const { return m_vertex_buffer; }
    /// @}

    /// @{
    /// OpenGL buffer storing the indicies.
    AnyIndexBufferPtr& get_indices() { return m_index_buffer; }
    const AnyIndexBufferPtr& get_indices() const { return m_index_buffer; }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Human-readable name of this VertexObject.
    std::string m_name;

    /// OpenGL buffer storing the vertices.
    AnyVertexBufferPtr m_vertex_buffer;

    /// OpenGL buffer storing the indicies.
    AnyIndexBufferPtr m_index_buffer;

    /// Id of the OpenGL VAO.
    VertexObjectId m_id = VertexObjectId::invalid();
};

NOTF_CLOSE_NAMESPACE
