#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// any index buffer ================================================================================================= //

struct AnyIndexBuffer {

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;
};

// index buffer ===================================================================================================== //

/// IndexBuffer ID type.
using IndexBufferId = detail::OpenGLBufferType<detail::AnyOpenGLBuffer::Type::INDEX>;

/// Abstraction of an OpenGL index buffer.
template<class IndexType>
class IndexBuffer : public AnyIndexBuffer, //
                    public OpenGLBuffer<detail::AnyOpenGLBuffer::Type::INDEX, IndexType> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::AnyOpenGLBuffer::Type::INDEX, IndexType>;

    /// Value type of the indices.
    using index_t = IndexType;

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
};

/// IndexBuffer factory.
/// @param name         Human-readable name of this OpenGLBuffer.
/// @param usage_hint   The expected usage of the data stored in this buffer.
/// @throws OpenGLError If the buffer could not be allocated.
template<class IndexType = GLuint>
auto create_index_buffer(std::string name,
                         const AnyIndexBuffer::UsageHint usage_hint = AnyIndexBuffer::UsageHint::DEFAULT) {
    return IndexBuffer<IndexType>::create(std::move(name), usage_hint);
}

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyUniformBufferPtr subclasses.
// template<class Lhs, class Rhs>
// struct std::common_type<::notf::UniformBufferPtr<Lhs>, ::notf::UniformBufferPtr<Rhs>> {
//    using type = ::notf::AnyUniformBufferPtr;
//};
