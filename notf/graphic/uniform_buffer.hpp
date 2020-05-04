#pragma once

#include "notf/meta/assert.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// uniform buffer =================================================================================================== //

/// Abstraction of an OpenGL uniform buffer.
template<class Block>
class UniformBuffer : public OpenGLBuffer<detail::OpenGLBufferType::UNIFORM, Block> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of UniformBlock stored in the UniformBuffer.
    using block_t = Block;

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;

private:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::OpenGLBufferType::UNIFORM, Block>;

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
    static UniformBufferPtr<Block> create(std::string name, const UsageHint usage_hint) {
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

/// UniformBuffer factory.
/// @param name         Human-readable name of this OpenGLBuffer.
/// @param usage_hint   The expected usage of the data stored in this buffer.
/// @throws OpenGLError If the buffer could not be allocated.
template<class Block>
auto make_uniform_buffer(std::string name,
                         const AnyUniformBuffer::UsageHint usage_hint = AnyUniformBuffer::UsageHint::DEFAULT) {
    return UniformBuffer<Block>::create(std::move(name), usage_hint);
}

/// UniformBuffer type produced by `make_uniform_buffer` with the given template arguments.
template<class Block>
using uniform_buffer_t = typename decltype(make_uniform_buffer<Block>(""))::element_type;

NOTF_CLOSE_NAMESPACE
