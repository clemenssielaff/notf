#pragma once

#include "notf/meta/macros.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/fwd.hpp"
#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// any uniform buffer =============================================================================================== //

namespace detail {

class AnyUniformBuffer {

    friend Accessor<AnyUniformBuffer, ShaderProgram>;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Default constructor.
    AnyUniformBuffer(std::string name);

public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(AnyUniformBuffer);

    NOTF_NO_COPY_OR_ASSIGN(AnyUniformBuffer);

    /// Destructor.
    virtual ~AnyUniformBuffer();

    /// OpenGL ID of the Uniform Buffer object.
    UniformBufferId get_id() const { return m_id; }

    /// Size of a single Block in this UniformBuffer in bytes.
    virtual size_t get_block_size() const = 0;

    /// Number of blocks stored in this UniformBuffer
    virtual size_t get_block_count() const = 0;

    /// Name of this UniformBuffer.
    const std::string& get_name() const { return m_name; }

    /// Slot that this uniform buffer is bound to in the GraphicsContext.
    GLuint get_bound_slot() const { return m_bound_slot; }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Name of this UniformBuffer.
    std::string m_name;

    /// OpenGL ID of the managed uniform buffer object.
    UniformBufferId m_id = 0;

    /// Slot that this uniform buffer is bound to in the GraphicsContext.
    GLuint m_bound_slot = 0;
};

} // namespace detail

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<detail::AnyUniformBuffer, ShaderProgram> {
    friend ShaderProgram;

    /// Binds the UniformBuffer to the given slot.
    /// @param buffer   UniformBuffer to bind.
    /// @param slot     Slot to bind to.
    static void bind(detail::AnyUniformBuffer& buffer, const GLuint slot) {
        if (slot != buffer.m_bound_slot) {
            NOTF_CHECK_GL(glBindBufferBase(GL_UNIFORM_BUFFER, buffer.m_bound_slot, slot));
            buffer.m_bound_slot = slot;
        }
    }

    /// Unbinds the UniformBuffer.
    /// @param buffer   UniformBuffer to unbind.
    static void unbind(detail::AnyUniformBuffer& buffer) {
        if (buffer.m_bound_slot) { NOTF_CHECK_GL(glBindBufferBase(GL_UNIFORM_BUFFER, buffer.m_bound_slot, 0)); }
    }
};

// uniform buffer =================================================================================================== //

/// Abstraction of an OpenGL uniform buffer.
template<class Block>
class UniformBuffer : public detail::AnyUniformBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of UniformBlock stored in the UniformBuffer.
    using block_t = Block;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(UniformBuffer);

    /// Private constructor.
    UniformBuffer(std::string name) : detail::AnyUniformBuffer(std::move(name)) {}

public:
    /// Factory.
    static auto create(std::string name) { return _create_shared<UniformBuffer<Block>>(std::move(name)); }

    /// Size of a single Block in this UniformBuffer in bytes.
    size_t get_block_size() const final { return sizeof(Block); }

    size_t get_block_count() const final { return m_buffer.size(); }

    std::vector<Block>& write() { return m_buffer; }

    void apply() {
        if (m_buffer.empty()) { return; }

        NOTF_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, m_id.value()));

        static const GLint alignment = _get_alignment();
        const GLsizei buffer_size = m_buffer.size() * alignment;
        if (buffer_size <= m_server_size) {
            NOTF_CHECK_GL(glBufferSubData(GL_UNIFORM_BUFFER, /*offset = */ 0, buffer_size, &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(glBufferData(GL_UNIFORM_BUFFER, buffer_size, &m_buffer.front(), GL_STREAM_DRAW));
            m_server_size = buffer_size;
        }

        NOTF_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    }

    void bind_index(const GLuint index) { glBindBufferBase(GL_UNIFORM_BUFFER, index, m_id.value()); }

private:
    static GLint _get_alignment() {
        GLint alignment;
        NOTF_CHECK_GL(glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment));
        return sizeof(Block) + alignment - sizeof(Block) % alignment;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Buffered data.
    std::vector<Block> m_buffer;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_server_size = 0;
};

NOTF_CLOSE_NAMESPACE
