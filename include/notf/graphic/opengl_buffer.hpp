#pragma once

#include "notf/graphic/opengl.hpp"

NOTF_OPEN_NAMESPACE

// any opengl buffer ================================================================================================ //

namespace detail {

class AnyOpenGLBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// OpenGL buffer type.
    using Type = detail::OpenGLBufferType;

    /// The expected usage of the data stored in this buffer.
    enum class UsageHint : GLushort {
        DYNAMIC_DRAW, ///< Written many times, read many times by the GPU (default)
        DYNAMIC_READ, ///< Written many times, read many times from the application
        DYNAMIC_COPY, ///< Written many times, read many times from the application as a source for new writes
        STATIC_DRAW,  ///< Written once, read many times from the GPU
        STATIC_READ,  ///< Written once, read many times from the application
        STATIC_COPY,  ///< Written once, read many times from the application as a source for new writes
        STREAM_DRAW,  ///< Written once, read only a few times by the GPU
        STREAM_READ,  ///< Written once, read only a few times from the application
        STREAM_COPY,  ///< Written once, read only a few times from the application as a source for new writes
        DEFAULT = DYNAMIC_DRAW,
    };

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @param buffer_type  OpenGL buffer type.
    /// @throws OpenGLError If the buffer could not be allocated.
    AnyOpenGLBuffer(std::string name, const UsageHint usage_hint, const Type buffer_type);

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyOpenGLBuffer);

    /// Destructor.
    virtual ~AnyOpenGLBuffer();

    /// Performs additional initialization of the buffer, should the type require it.
    virtual void initialize() { m_is_initialized = true; }

    /// Name of this OpenGLBuffer.
    const std::string& get_name() const { return m_name; }

    /// OpenGL buffer type.
    Type get_type() const { return m_type; }

    /// The expected usage of the data stored in this buffer.
    UsageHint get_usage_hint() const { return m_usage; }

    /// Checks if there is any data stored in this buffer.
    virtual bool is_empty() const = 0;

    /// Number of elements stored in this buffer.
    virtual size_t get_element_count() const = 0;

    /// Size of an element in this buffer (including padding) in bytes.
    virtual size_t get_element_size() const = 0;

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method does nothing.
    virtual void upload() = 0;

protected:
    /// Numeric OpenGL handle of this buffer.
    GLuint _get_handle() const { return m_handle; }

    /// Whether or not `_initialize` has been called or not.
    bool _is_initialized() const { return m_is_initialized; }

    /// Produces the name of a buffer type from its OpenGL enum value.
    /// @param buffer_type  Buffer type to convert.
    /// @returns            Name of the buffer type or nullptr on error.
    static const char* _to_type_name(const Type buffer_type);

    /// Produces the OpenGL buffer typec corresponding to the given Type value.
    /// @param buffer_type  Buffer type to convert.
    /// @returns            OpenGL buffer type enum value.
    static GLenum _to_gl_type(const Type buffer_type);

    /// Produces the OpenGL enum value corresponding to the given usage hint.
    /// @param usage    The expected usage of the data stored in this buffer.
    /// @returns        OpenGL enum value.
    static GLenum _to_gl_usage(const UsageHint usage);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Human-readable name of this OpenGLBuffer.
    std::string m_name;

    /// Numeric OpenGL handle of this buffer.
    GLuint m_handle = 0;

    /// The expected usage of the data stored in this buffer.
    UsageHint m_usage = UsageHint::DEFAULT;

    /// Whether or not `_initialize` has been called or not.
    bool m_is_initialized = false;

    /// OpenGL buffer type.
    const Type m_type;
};

// typed opengl buffer ============================================================================================== //

/// Typed but virtual OpenGL Buffer type.
template<detail::OpenGLBufferType t_buffer_type>
class TypedOpenGLBuffer : public AnyOpenGLBuffer {


    // types ----------------------------------------------------------------------------------- //
public:
    /// ID type used to identify this OpenGL buffer.
    using id_t = IdType<detail::TypedOpenGLBuffer<t_buffer_type>, GLuint>;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    TypedOpenGLBuffer(std::string name, const UsageHint usage_hint)
        : AnyOpenGLBuffer(std::move(name), usage_hint, t_buffer_type) {}

public:
    /// Typed ID of this buffer.
    id_t get_id() const { return this->_get_handle(); }
};

} // namespace detail

// opengl buffer ==================================================================================================== //

template<detail::OpenGLBufferType t_buffer_type, class Data>
class OpenGLBuffer : public detail::TypedOpenGLBuffer<t_buffer_type> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of data stored in this OpenGL buffer.
    using data_t = Data;

    /// OpenGL buffer type.
    using Type = typename detail::OpenGLBufferType;

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Human-readable name of this OpenGLBuffer.
    /// @param usage_hint   The expected usage of the data stored in this buffer.
    /// @throws OpenGLError If the buffer could not be allocated.
    OpenGLBuffer(std::string name, const UsageHint usage_hint = UsageHint::DEFAULT)
        : detail::TypedOpenGLBuffer<t_buffer_type>(std::move(name), usage_hint) {}

public:
    /// Checks if there is any data stored in this buffer.
    bool is_empty() const final { return m_buffer.empty(); }

    /// Number of elements stored in this buffer.
    size_t get_element_count() const final { return m_buffer.size(); }

    /// Size of an element in this buffer (including padding) in bytes.
    size_t get_element_size() const override { return sizeof(data_t); }

    /// Write-access to the data stored in this buffer.
    auto& write() {
        m_local_hash = 0;
        // TODO: OpeGLBuffer::write should return a dedicated "write" object, that keeps track of changes.
        // This way, it can determine whether the local hash needs to be re-calculated in `upload` or not (by setting
        // `m_local_hash` to zero on each change) and we might even be able to use multiple smaller calls to
        // `glBufferSubData` instead of a single big one, just from data we collect automatically from the writer.
        return m_buffer;
    }

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method does nothing.
    void upload() final {
        if (is_empty()) { return; }

        // update the local hash on request
        if (0 == m_local_hash) { m_local_hash = hash(m_buffer); }

        // do nothing if the data on the server is still current
        if (m_local_hash == m_server_hash) { return; }

        // bind and eventually unbind the index buffer
        const GLenum gl_type = this->_to_gl_type(this->get_type());
        struct BufferGuard {
            BufferGuard(const OpenGLBuffer& buffer) {
                const Type type = buffer.get_type();
                // vertex- and index-buffers should be part of a VertexObject (VAO) that takes care of un/binding
                if (type != OpenGLBuffer::Type::VERTEX && //
                    type != OpenGLBuffer::Type::INDEX) {
                    m_type = buffer._to_gl_type(type);
                    NOTF_CHECK_GL(glBindBuffer(m_type, buffer._get_handle()));
                }
            }
            ~BufferGuard() {
                if (m_type != 0) { NOTF_CHECK_GL(glBindBuffer(m_type, 0)); }
            }
            GLenum m_type = 0;
        };
        NOTF_GUARD(BufferGuard(*this));

        // upload the buffer data
        const GLsizei buffer_size = narrow_cast<GLsizei>(m_buffer.size() * get_element_size());
        if (buffer_size <= m_server_size) {
            NOTF_CHECK_GL(glBufferSubData(gl_type, /*offset = */ 0, buffer_size, &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(
                glBufferData(gl_type, buffer_size, &m_buffer.front(), this->_to_gl_usage(this->get_usage_hint())));
            m_server_size = buffer_size;
        }
        m_server_hash = m_local_hash;

        // TODO: It might be better to use two buffers for each VertexBuffer object.
        // One that is currently rendered from, one that is written into. Note on the OpenGL reference:
        // (https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBufferSubData.xhtml)
        //
        //      Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store
        //      updates. If any rendering in the pipeline makes reference to data in the buffer object being updated by
        //      glBufferSubData, especially from the specific region being updated, that rendering must drain from the
        //      pipeline before the data store can be updated.
        //
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Local buffer.
    std::vector<data_t> m_buffer;

    /// Size in bytes of the buffer allocated on the server.
    size_t m_server_size = 0;

    /// Hash of the current data held by the application.
    size_t m_local_hash = 0;

    /// Hash of the data that was last uploaded to the server.
    size_t m_server_hash = 0;
};

NOTF_CLOSE_NAMESPACE
