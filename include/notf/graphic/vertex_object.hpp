#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/index_buffer.hpp"
#include "notf/graphic/vertex_buffer.hpp"

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

private:
    /// RAII guard to make sure that the VAO is properly unbound again.
    struct VaoGuard {
        VaoGuard(const GLuint vao_id) { NOTF_CHECK_GL(glBindVertexArray(vao_id)); }
        ~VaoGuard() { NOTF_CHECK_GL(glBindVertexArray(0)); }
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexObject);

    /// Constructor.
    /// @param context          GraphicsContext that his VertexObject lives in.
    /// @param name             Human-readable name of this VertexObject.
    /// @throws OpenGLError     If the VAO could not be allocated.
    VertexObject(GraphicsContext& context, std::string name);

public:
    NOTF_NO_COPY_OR_ASSIGN(VertexObject);

    /// Factory.
    /// @param context          GraphicsContext that his VertexObject lives in.
    /// @param name             Human-readable name of this VertexObject.
    /// @throws OpenGLError     If the VAO could not be allocated.
    static VertexObjectPtr create(GraphicsContext& context, std::string name);

    /// Destructor.
    ~VertexObject() { _deallocate(); }

    /// (Re-)Bind an IndexBuffer to this VertexObject.
    /// Only one IndexBuffer can be bound to a VertexObject at any time.
    /// @param index_buffer IndexBuffer to bind.
    template<class IndexType>
    void bind(IndexBufferPtr<IndexType> index_buffer) {
        if (!index_buffer) { return; }
        {
            NOTF_GUARD(VaoGuard(m_id.get_value()));
            IndexBuffer<IndexType>::template AccessFor<VertexObject>::bind_to_vao(*index_buffer.get());
        }
        m_index_buffer = std::move(index_buffer);
    }

    /// Bind a new VertexBuffer to this VertexObject.
    /// @param vertex_buffer VertexBuffer to bind.
    template<class AttributePolicies, class Vertex, class... Indices,
             class = std::enable_if_t<all(sizeof...(Indices) == std::tuple_size_v<Vertex>, //
                                          all_convertible_to_v<uint, Indices...>)>>
    void bind(VertexBufferPtr<AttributePolicies, Vertex> vertex_buffer, Indices... indices) {
        if (!vertex_buffer) { return; }
        {
            NOTF_GUARD(VaoGuard(m_id.get_value()));
            VertexBuffer<AttributePolicies, Vertex>::template AccessFor<VertexObject>::bind_to_vao(
                *vertex_buffer.get(), std::forward<Indices>(indices)...);
        }
        m_vertex_buffers.emplace_back(std::move(vertex_buffer));
    }

    /// Checks if the VertexObject is valid.
    /// If the GraphicsContext managing this VertexObject is destroyed while there is still one or more`shared_ptr`s
    /// to this VertexObject, the VertexObject will become invalid and all attempts of modification will throw.
    bool is_valid() const { return m_id.is_valid(); }

    /// GraphicsContext that his VertexObject lives in.
    GraphicsContext& get_context() { return m_context; }

    /// Human-readable name of this VertexObject.
    const std::string& get_name() const { return m_name; }

    /// Id of the OpenGL VAO.
    VertexObjectId get_id() const { return m_id; }

    //    /// @{
    //    /// OpenGL buffer storing the vertices.
    //    AnyVertexBufferPtr& get_vertices() { return m_vertex_buffer; }
    //    const AnyVertexBufferPtr& get_vertices() const { return m_vertex_buffer; }
    //    /// @}

    //    /// @{
    //    /// OpenGL buffer storing the indicies.
    //    AnyIndexBufferPtr& get_indices() { return m_index_buffer; }
    //    const AnyIndexBufferPtr& get_indices() const { return m_index_buffer; }
    //    /// @}

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
    std::vector<AnyVertexBufferPtr> m_vertex_buffers;

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
