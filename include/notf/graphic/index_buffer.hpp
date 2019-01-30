#pragma once

#include "notf/meta/numeric.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

static_assert(std::is_same_v<size_t, uint64_t>);

/// Finds the smallest OpenGL unsigned integer type capable of representing the given value.
/// Is either GLubyte, GLushort, GLuint or GLuint64.
template<size_t Size>
class gl_smallest_unsigned_type {

    static constexpr auto detect() {
        if constexpr (Size <= max_value<GLubyte>()) {
            return GLubyte();
        } else if constexpr (Size <= max_value<GLushort>()) {
            return GLushort();
        } else if constexpr (Size <= max_value<GLuint>()) {
            return GLuint();
        } else if constexpr (Size <= max_value<GLuint64>()) {
            return GLuint64();
        }
    }

public:
    using type = std::decay_t<decltype(detect())>;
};

} // namespace detail

// any index buffer ================================================================================================= //

/// Base class for all IndexBuffers.
/// See `IndexBuffer` for implementation details.
class AnyIndexBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Arguments for the IndexBuffer.
    struct Args {
        /// The expected usage of the data.
        GLUsage usage = GLUsage::DEFAULT;
    };

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Name of this IndexBuffer.
    /// @param args         Arguments used to initialize this IndexBuffer.
    /// @throws OpenGLError If the EAB could not be allocated.
    AnyIndexBuffer(std::string name, Args args);

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyIndexBuffer);

    /// Destructor.
    virtual ~AnyIndexBuffer();

    /// Name of this IndexBuffer.
    const std::string& get_name() const { return m_name; }

    /// OpenGL handle of the index buffer.
    IndexBufferId get_id() const { return m_id; }

    /// Checks if there are any indices stored in this Buffer.
    virtual bool is_empty() const = 0;

protected:
    /// Registers a new IndexBuffer with the ResourceManager.
    static void _register_ressource(AnyIndexBufferPtr index_buffer);

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Arguments used to initialize the IndexBuffer.
    const Args m_args;

    /// Name of this IndexBuffer.
    std::string m_name;

    /// OpenGL ID of the managed element array buffer.
    IndexBufferId m_id = IndexBufferId::invalid();
};

// index buffer ======================================================================================================
// //

/// Abstraction of an OpenGL index buffer.
template<class IndexType>
class IndexBuffer : public AnyIndexBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Value type of the indices.
    using index_t = IndexType;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(IndexBuffer);

    /// Constructor.
    /// @param name         Name of this IndexBuffer.
    /// @param args         Arguments used to initialize this IndexBuffer.
    /// @throws OpenGLError If the EAB could not be allocated.
    IndexBuffer(std::string name, Args args) : AnyIndexBuffer(std::move(name), std::move(args)) {}

public:
    /// Factory.
    /// @param name             Name of this IndexBuffer.
    /// @param args             Arguments used to initialize this IndexBuffer.
    /// @throws OpenGLError     If the EAB could not be allocated.
    /// @throws ResourceError   If another IndexBuffer with the same name already exist.
    static IndexBufferPtr<IndexType> create(std::string name, Args args) {
        auto result = _create_shared(std::move(name), std::move(args));
        _register_ressource(result);
        return result;
    }

    /// Checks if there are any indices stored in this Buffer.
    bool is_empty() const final { return m_buffer.empty(); }

    /// Write-access to the index buffer.
    std::vector<index_t>& write() {
        m_local_hash = 0;
        // TODO: IndexBuffer::write should return a dedicated "write" object, that keeps track of changes.
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
//            m_local_hash = hash(m_buffer); // TODO:NOW hash index buffer

            // noop if the data on the server is still current
            if (m_local_hash == m_server_hash) { return; }
        }

        // bind and eventually unbind the index buffer
        struct IndexBufferGuard {
            IndexBufferGuard(const GLuint id) { NOTF_CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id)); }
            ~IndexBufferGuard() { NOTF_CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)); }
        };
        NOTF_GUARD(IndexBufferGuard(get_id().get_value()));

        // upload the buffer data
        const GLsizei buffer_size = m_buffer.size() * sizeof(index_t);
        if (buffer_size <= m_server_size) {
            NOTF_CHECK_GL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, /*offset = */ 0, buffer_size, &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, buffer_size, &m_buffer.front(), get_gl_usage(m_args.usage)));
            m_server_size = buffer_size;
        }
        // TODO: It might be better to use two buffers for each VertexBuffer object.
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

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Indices stored in the buffer.
    std::vector<index_t> m_buffer;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_server_size = 0;

    /// Hash of the current data held by the application.
    size_t m_local_hash = 0;

    /// Hash of the data that was last uploaded to the GPU.
    size_t m_server_hash = 0;
};

/// Creates an IndexArray object containing the given indices in their smalles representable form.
/// Passing an index less than zero or larger than a GLuint64 raises an error during compilation.
/// The returned IndexBuffer is not applied yet.
template<size_t... Indices>
auto create_index_buffer() {
    using value_t = typename detail::gl_smallest_unsigned_type<std::max<size_t>({Indices...})>::type;
    auto result = std::make_unique<IndexBuffer<value_t>>();
    result->buffer() = std::vector<value_t>{Indices...};
    return result;
}

NOTF_CLOSE_NAMESPACE
