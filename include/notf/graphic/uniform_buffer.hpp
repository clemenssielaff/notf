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

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name Name of this UniformBuffer.
    AnyUniformBuffer(std::string name); // TODO: uniform buffer names (as well ass all others) should be resource-unique

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyUniformBuffer);

    /// Destructor.
    virtual ~AnyUniformBuffer();

    /// Name of this UniformBuffer.
    const std::string& get_name() const { return m_name; }

    /// OpenGL ID of the Uniform Buffer object.
    UniformBufferId get_id() const { return m_id; }

    /// Size of a single Block in this UniformBuffer in bytes.
    virtual size_t get_block_size() const = 0;

    /// Number of blocks stored in this UniformBuffer
    virtual size_t get_block_count() const = 0;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Name of this UniformBuffer.
    const std::string m_name;

    /// OpenGL ID of the managed uniform buffer object.
    UniformBufferId m_id = 0;
};

} // namespace detail

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
    size_t get_block_size() const final {
        static const size_t block_size = static_cast<size_t>(_get_block_size());
        return block_size;
    }

    size_t get_block_count() const final { return m_buffer.size(); }

    std::vector<Block>& write() {
        m_local_hash = 0;
        // TODO: UniformBuffer::write should return a dedicated "write" object, that keeps track of changes.
        // This way, it can determine whether the local hash needs to be re-calculated in `apply` or not (by setting
        // `m_local_hash` to zero on each change) and we might even be able to use multiple smaller calls to
        // `glBufferSubData` instead of a single big one, just from data we collect automatically from the writer.
        return m_buffer;
    }

    void apply() {
        // noop if there is nothing to update
        if (m_buffer.empty()) { return; }

        // update the local hash on request
        if (0 == m_local_hash) {
            m_local_hash = hash(m_buffer);

            // noop if the data on the server is still current
            if (m_local_hash == m_server_hash) { return; }
        }

        // bind and eventually unbind the uniform buffer
        struct UniformBufferGuard {
            UniformBufferGuard(const GLuint id) { NOTF_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, id)); }
            ~UniformBufferGuard() { NOTF_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0)); }
        };
        NOTF_GUARD(UniformBufferGuard(get_id().get_value()));

        // upload the buffer data
        static const GLint block_size = _get_block_size();
        const GLsizei buffer_size = m_buffer.size() * block_size;
        if (buffer_size <= m_server_size) {
            NOTF_CHECK_GL(glBufferSubData(GL_UNIFORM_BUFFER, /*offset = */ 0, buffer_size, &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(glBufferData(GL_UNIFORM_BUFFER, buffer_size, &m_buffer.front(), GL_STREAM_DRAW));
            m_server_size = buffer_size;
        }
        // TODO: It might be better to use two buffers for each UniformBuffer object.
        // One that is currently rendered from, one that is written into. Note on the OpenGL reference:
        // (https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBufferSubData.xhtml)
        //
        //      Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store
        //      updates. If any rendering in the pipeline makes reference to data in the buffer object being updated by
        //      glBufferSubData, especially from the specific region being updated, that rendering must drain from the
        //      pipeline before the data store can be updated.
        //

        m_server_hash = m_local_hash;
    }

private:
    /// Calculates the actual, aligned size of a block when stored in GPU memory.
    static GLint _get_block_size() {
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

    size_t m_local_hash = 0;

    size_t m_server_hash = 0;
};

NOTF_CLOSE_NAMESPACE
