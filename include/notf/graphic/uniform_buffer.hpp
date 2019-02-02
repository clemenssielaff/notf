#pragma once

#include "notf/meta/assert.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// any uniform buffer =============================================================================================== //

struct AnyUniformBuffer {

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;
};

// uniform buffer =================================================================================================== //

/// UniformBuffer ID type.
using UniformBufferId = detail::OpenGLBufferType<detail::AnyOpenGLBuffer::Type::UNIFORM>;

/// Abstraction of an OpenGL uniform buffer.
template<class Block>
class UniformBuffer : public AnyUniformBuffer, //
                      public OpenGLBuffer<detail::AnyOpenGLBuffer::Type::UNIFORM, Block> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::AnyOpenGLBuffer::Type::UNIFORM, Block>;

    /// Type of UniformBlock stored in the UniformBuffer.
    using block_t = Block;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(UniformBuffer);

    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    UniformBuffer(std::string name, const UsageHint usage_hint) : super_t(std::move(name), usage_hint) {}


public:
    /// Factory.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    static auto create(std::string name, const UsageHint usage_hint) {
        return _create_shared(std::move(name), std::move(usage_hint));
    }

    /// Size of an element in this buffer (including padding) in bytes.
    size_t get_element_size() const override {
        static const size_t element_size = static_cast<size_t>(_get_block_size());
        return element_size;
    }

private:
    /// Calculates the actual, aligned size of a block when stored in GPU memory.
    static GLint _get_block_size() {
        GLint alignment = 0;
        NOTF_CHECK_GL(glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment));
        NOTF_ASSERT(alignment);
        return sizeof(Block) + alignment - sizeof(Block) % alignment;
    }
};

/// IndexBuffer factory.
/// @param name         Human-readable name of this OpenGLBuffer.
/// @param usage_hint   The expected usage of the data stored in this buffer.
/// @throws OpenGLError If the buffer could not be allocated.
template<class Block>
auto create_uniform_buffer(std::string name,
                         const AnyUniformBuffer::UsageHint usage_hint = AnyUniformBuffer::UsageHint::DEFAULT) {
    return UniformBuffer<Block>::create(std::move(name), usage_hint);
}

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyUniformBufferPtr subclasses.
// template<class Lhs, class Rhs>
// struct std::common_type<::notf::UniformBufferPtr<Lhs>, ::notf::UniformBufferPtr<Rhs>> {
//    using type = ::notf::AnyUniformBufferPtr;
//};
