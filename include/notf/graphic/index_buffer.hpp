#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// index buffer ===================================================================================================== //

/// Abstraction of an OpenGL index buffer.
template<class IndexType>
class IndexBuffer : public OpenGLBuffer<detail::OpenGLBufferType::INDEX, IndexType> {

    friend Accessor<IndexBuffer, VertexObject>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(IndexBuffer);

    /// Value type of the indices.
    using index_t = IndexType;

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;

private:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::OpenGLBufferType::INDEX, IndexType>;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(IndexBuffer);

    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    IndexBuffer(std::string name, const UsageHint usage_hint) : super_t(std::move(name), usage_hint) {}

public:
    /// Factory.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    static auto create(std::string name, const UsageHint usage_hint) {
        return _create_shared(std::move(name), std::move(usage_hint));
    }

private:
    /// Binds the IndexBuffer to the bound VertexObject.
    /// @throws OpenGLError If no VAO is bound.
    void _bind_to_vao() {
        { // make sure there is a bound VertexObject
            GLint current_vao = 0;
            NOTF_CHECK_GL(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(OpenGLError, "Cannot initialize an IndexBuffer without an active VertexObject");
            }
        }
        NOTF_CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_get_handle()));
    }
};

/// IndexBuffer factory.
/// @param name         Human-readable name of this OpenGLBuffer.
/// @param usage_hint   The expected usage of the data stored in this buffer.
/// @throws OpenGLError If the buffer could not be allocated.
template<class IndexType = GLuint>
auto make_index_buffer(std::string name,
                       const AnyIndexBuffer::UsageHint usage_hint = AnyIndexBuffer::UsageHint::DEFAULT) {
    return IndexBuffer<IndexType>::create(std::move(name), usage_hint);
}

/// IndexBuffer type produced by `make_index_buffer` with the given template arguments.
template<class IndexType>
using index_buffer_t = typename decltype(make_index_buffer<IndexType>(""))::element_type;

// accessors ======================================================================================================== //

template<class IndexType>
class Accessor<IndexBuffer<IndexType>, VertexObject> {
    friend VertexObject;

    /// Binds the IndexBuffer to the bound VertexObject.
    /// @throws OpenGLError If no VAO is bound.
    static void bind_to_vao(IndexBuffer<IndexType>& buffer) { buffer._bind_to_vao(); }
};

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyIndexBufferPtr subclasses.
template<class Lhs, class Rhs>
struct std::common_type<::notf::IndexBufferPtr<Lhs>, ::notf::IndexBufferPtr<Rhs>> {
    using type = ::notf::AnyIndexBufferPtr;
};
