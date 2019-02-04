#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// index buffer ===================================================================================================== //

/// Abstraction of an OpenGL index buffer.
template<class IndexType>
class IndexBuffer : public OpenGLBuffer<detail::OpenGLBufferType::INDEX, IndexType> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::OpenGLBufferType::INDEX, IndexType>;

    /// Value type of the indices.
    using index_t = IndexType;

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(IndexBuffer);

    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    IndexBuffer(std::string name, const UsageHint usage_hint) : super_t(std::move(name), usage_hint) {}

    /// Performs additional initialization of the buffer, should the type require it.
    void initialize() final {
        if (this->_is_initialized()) { return; }

        if constexpr (config::is_debug_build()) { // make sure there is a bound VertexObject
            GLint current_vao = 0;
            NOTF_CHECK_GL(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(OpenGLError, "Cannot initialize an IndexBuffer without an active VertexObject");
            }
        }

        NOTF_CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_get_handle()));

        detail::AnyOpenGLBuffer::initialize();
    }

public:
    /// Factory.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    static auto create(std::string name, const UsageHint usage_hint) {
        return _create_shared(std::move(name), std::move(usage_hint));
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

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyIndexBufferPtr subclasses.
template<class Lhs, class Rhs>
struct std::common_type<::notf::IndexBufferPtr<Lhs>, ::notf::IndexBufferPtr<Rhs>> {
    using type = ::notf::AnyIndexBufferPtr;
};
