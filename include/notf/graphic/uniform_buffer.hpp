#pragma once

#include <vector>

#include "notf/meta/assert.hpp"
#include "notf/meta/macros.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/types.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// any uniform buffer =============================================================================================== //

/// Base class for all UniformBufffers.
/// See `UniformBuffer` for implementation details.
class AnyUniformBuffer {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Name of this UniformBuffer.
    /// @throws OpenGLError If the UBO could not be allocated.
    AnyUniformBuffer(std::string name);

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

    /// Number of blocks stored in this UniformBuffer.
    virtual size_t get_block_count() const = 0;

protected:
    /// Registers a new UniformBuffer with the ResourceManager.
    static void _register_ressource(AnyUniformBufferPtr uniform_buffer);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Name of this UniformBuffer.
    const std::string m_name;

    /// OpenGL ID of the managed uniform buffer object.
    UniformBufferId m_id = 0;
};

// uniform buffer =================================================================================================== //

/// Abstraction of an OpenGL uniform buffer.
template<class Block>
class UniformBuffer : public AnyUniformBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of UniformBlock stored in the UniformBuffer.
    using block_t = Block;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(UniformBuffer);

    /// Constructor.
    /// @param name         Name of this UniformBuffer.
    /// @throws OpenGLError If the UBO could not be allocated.
    UniformBuffer(std::string name) : AnyUniformBuffer(std::move(name)) {}

public:
    /// Factory.
    /// @param name             Name of this UniformBuffer.
    /// @throws OpenGLError     If the UBO could not be allocated.
    /// @throws ResourceError   If another UniformBlock with the same name already exist.
    static UniformBufferPtr<Block> create(std::string name) {
        auto result = _create_shared<UniformBuffer>(std::move(name));
        _register_ressource(result);
        return result;
    }

    /// Size of a single Block in this UniformBuffer in bytes.
    size_t get_block_size() const final {
        static const size_t block_size = static_cast<size_t>(_get_block_size());
        return block_size;
    }

    /// Number of blocks stored in the buffer.
    size_t get_block_count() const final { return m_buffer.size(); }

    std::vector<Block>& write() {
        m_local_hash = 0;
        // TODO: UniformBuffer::write should return a dedicated "write" object, that keeps track of changes.
        // This way, it can determine whether the local hash needs to be re-calculated in `apply` or not (by setting
        // `m_local_hash` to zero on each change) and we might even be able to use multiple smaller calls to
        // `glBufferSubData` instead of a single big one, just from data we collect automatically from the writer.
        return m_buffer;
    }

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method is a noop.
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
        GLint alignment = 0;
        NOTF_CHECK_GL(glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment));
        NOTF_ASSERT(alignment);
        return sizeof(Block) + alignment - sizeof(Block) % alignment;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Buffered data.
    std::vector<Block> m_buffer;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_server_size = 0;

    /// Hash of the current data held by the application.
    size_t m_local_hash = 0;

    /// Hash of the data that was last uploaded to the GPU.
    size_t m_server_hash = 0;
};

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyUniformBufferPtr subclasses.
template<class Lhs, class Rhs>
struct std::common_type<::notf::UniformBufferPtr<Lhs>, ::notf::UniformBufferPtr<Rhs>> {
    using type = ::notf::AnyUniformBufferPtr;
};
