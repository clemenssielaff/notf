#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// vertex object ==================================================================================================== //

/// VertexObjects are owned by a GraphicContext, and managed by the user through `shared_ptr`s. The
/// VertexObject is deallocated when the last `shared_ptr` goes out of scope or the associated GraphicsContext is
/// deleted, whatever happens first. Trying to modify a `shared_ptr` to a deallocated VertexObject will throw an
/// exception.
class VertexObject {

    friend Accessor<VertexObject, GraphicsContext>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(VertexObject);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexObject);

    /// Constructor.
    /// @param context          GraphicsContext that his VertexObject lives in.
    /// @param name             Human-readable name of this VertexObject.
    /// @param vertex_buffer    OpenGL buffer storing the vertices.
    /// @param index_buffer     OpenGL buffer storing the indicies.
    /// @throws ValueError      If any of the given buffers are null.
    /// @throws OpenGLError     If the VAO could not be allocated.
    VertexObject(GraphicsContext& context, std::string name, AnyVertexBufferPtr vertex_buffer,
                 AnyIndexBufferPtr index_buffer);

public:
    NOTF_NO_COPY_OR_ASSIGN(VertexObject);

    /// Factory.
    /// @param context          GraphicsContext that his VertexObject lives in.
    /// @param name             Human-readable name of this VertexObject.
    /// @param vertex_buffer    OpenGL buffer storing the vertices.
    /// @param index_buffer     OpenGL buffer storing the indicies.
    /// @throws ValueError      If any of the given buffers are null.
    /// @throws OpenGLError     If the VAO could not be allocated.
    static VertexObjectPtr create(GraphicsContext& context, std::string name, AnyVertexBufferPtr vertex_buffer,
                                  AnyIndexBufferPtr index_buffer);

    /// Destructor.
    ~VertexObject() { _deallocate(); }

    /// Checks if the VertexObject is valid.
    /// If the GraphicsContext managing this VertexObject is destroyed while there is still one or more`shared_ptr`s to
    /// this VertexObject, the VertexObject will become invalid and all attempts of modification will throw.
    bool is_valid() const { return m_id.is_valid(); }

    /// GraphicsContext that his VertexObject lives in.
    GraphicsContext& get_context() { return m_context; }

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

private:
    /// Deallocates the VAO data and invalidates the VertexObject.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// GraphicsContext managing this VertexObject.
    GraphicsContext& m_context;

    /// Human-readable name of this VertexObject.
    std::string m_name;

    /// OpenGL buffer storing the vertices.
    AnyVertexBufferPtr m_vertex_buffer;

    /// OpenGL buffer storing the indicies.
    AnyIndexBufferPtr m_index_buffer;

    /// Id of the OpenGL VAO.
    VertexObjectId m_id = VertexObjectId::invalid();
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<VertexObject, GraphicsContext> {
    friend GraphicsContext;

    /// GraphicsContext managing the given VertexObject.
    static const GraphicsContext* get_graphics_context(VertexObject& vertex_object) { return &vertex_object.m_context; }

    /// Deallocates the VAO data and invalidates the VertexObject.
    static void deallocate(VertexObject& vertex_object) { vertex_object._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
