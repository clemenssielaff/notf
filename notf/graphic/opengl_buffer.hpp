#pragma once

#include "notf/common/vector.hpp"

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

    /// Produces the OpenGL buffer typec corresponding to the given Type value.
    /// @param buffer_type  Buffer type to convert.
    /// @returns            OpenGL buffer type enum value.
    constexpr static GLenum to_gl_type(const Type buffer_type) {
        switch (buffer_type) {
        case Type::VERTEX: return GL_ARRAY_BUFFER;
        case Type::INDEX: return GL_ELEMENT_ARRAY_BUFFER;
        case Type::UNIFORM: return GL_UNIFORM_BUFFER;
        case Type::DRAWCALL: return GL_DRAW_INDIRECT_BUFFER;
        default: NOTF_THROW(ValueError, "Unknown OpenGLBuffer type");
        }
    }

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

    /// Name of this OpenGLBuffer.
    const std::string& get_name() const { return m_name; }

    /// OpenGL buffer type.
    Type get_type() const { return m_type; }

    /// The expected usage of the data stored in this buffer.
    UsageHint get_usage_hint() const { return m_usage; }

    /// Checks if there is any data stored in this buffer.
    virtual bool is_empty() const = 0;

    /// Number of elements stored in this buffer.
    virtual uint get_element_count() const = 0;

    /// Size of an element in this buffer (including padding) in bytes.
    virtual size_t get_element_size() const = 0;

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method does nothing.
    virtual void upload() = 0;

protected:
    /// Numeric OpenGL handle of this buffer.
    GLuint _get_handle() const { return m_handle; }

    /// Produces the name of a buffer type from its OpenGL enum value.
    /// @param buffer_type  Buffer type to convert.
    /// @returns            Name of the buffer type or nullptr on error.
    static const char* _to_type_name(const Type buffer_type);

    /// Produces the OpenGL enum value corresponding to the given usage hint.
    /// @param usage    The expected usage of the data stored in this buffer.
    /// @returns        OpenGL enum value.
    static GLenum _to_gl_usage(const UsageHint usage);

    /// Prints a log message informing about the size of the buffer after a resize.
    /// @param size New size of the buffer.
    void _log_buffer_size(const size_t size) const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Human-readable name of this OpenGLBuffer.
    std::string m_name;

    /// Numeric OpenGL handle of this buffer.
    GLuint m_handle = 0;

    /// The expected usage of the data stored in this buffer.
    UsageHint m_usage = UsageHint::DEFAULT;

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

// opengl buffer guard ============================================================================================== //

template<OpenGLBufferType t_buffer_type>
struct OpenGLBufferGuard {
    using buffer_t = TypedOpenGLBuffer<t_buffer_type>;
    OpenGLBufferGuard(const buffer_t& buffer) { NOTF_CHECK_GL(glBindBuffer(s_type, buffer.get_id().get_value())); }
    ~OpenGLBufferGuard();
    static const GLenum s_type = buffer_t::to_gl_type(t_buffer_type);
};

template<OpenGLBufferType t_buffer_type>
OpenGLBufferGuard<t_buffer_type>::~OpenGLBufferGuard() {
    NOTF_CHECK_GL(glBindBuffer(s_type, 0));
}
template<>
inline OpenGLBufferGuard<OpenGLBufferType::INDEX>::~OpenGLBufferGuard() {
    // do not unbind index buffers
}

} // namespace detail

// opengl buffer ==================================================================================================== //

template<detail::OpenGLBufferType t_buffer_type, class Data>
class OpenGLBuffer : public detail::TypedOpenGLBuffer<t_buffer_type> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of data stored in this OpenGL buffer.
    using data_t = Data;

    /// OpenGL buffer type.
    using Type = detail::OpenGLBufferType;

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
    uint get_element_count() const final { return narrow_cast<uint>(m_buffer.size()); }

    /// Size of an element in this buffer (including padding) in bytes.
    size_t get_element_size() const override { return sizeof(data_t); }

    /// Write-access to the data stored in this buffer.
    auto& write() {
        m_local_hash = 0;
        // TODO: OpeGLBuffer::write should return a dedicated "write" object, that keeps track of changes.
        // This way, it can determine whether the local hash needs to be re-calculated in `upload` or not (by setting
        // `m_local_hash` to zero on each change) and we might even be able to use multiple smaller calls to
        // `glBufferSubData` instead of a single big one, just from data we collect automatically from the writer.
        //
        // The write object should take the written data (either as object or as ptr+size) and return the location in
        // the buffer into which it was written. This way, we can optimize internally, for example by seeing if the
        // object is already in the buffer and if so, just returning the offset to the existing object instead of
        // inserting a new one
        return m_buffer;
    }

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method does nothing.
    void upload() final {
        if (is_empty()) { return; }

        const GLsizei buffer_size = m_buffer.size() * get_element_size();

        // update the local hash on request
        if (0 == m_local_hash) {

            // TODO: memory bug here
            // the problem is that `get_element_size` called from an uniform buffer produces the size of the data type
            // plus padding - but in memory, we store it without padding. That means that when we pass the memory on to
            // OpenGL, we will:
            //  1. pass more memory than is in the buffer
            //  2. from the second element ownwards, the offsets won't work and the data will be garbage
            // The next line is a hack fix for nr. 1 at least for now, where we only have a single entry.
            // THIS IS GARBAGE
            // What I really want is not a virtual `get_memory_size`, but that the OpenGL buffer takes a type (Data)
            // and produces its own type (data_t), which may or may not be the same as the one passed in.
            // Of course, the big problem is, that we don't know the size of a uniform block (inclusive padding) at
            // compile time ... otherwise, data_t could always be:
            //  data_t : public Data { /* with a padding array, if necessary */ }
            if (m_buffer.size() * sizeof(data_t) < static_cast<uint>(buffer_size)) {
                m_buffer.emplace_back(m_buffer.back());
            }

            m_local_hash = hash(m_buffer);
        }

        // do nothing if the data on the server is still current
        if (m_local_hash == m_server_hash) { return; }

        // bind and eventually unbind the index buffer
        NOTF_GUARD(detail::OpenGLBufferGuard(*this));

        // upload the buffer data
        const GLenum gl_type = this->to_gl_type(this->get_type());
        if (buffer_size <= narrow_cast<GLsizei>(m_server_size)) {
            NOTF_CHECK_GL(
                glBufferSubData(gl_type, /*offset = */ 0, narrow_cast<GLsizei>(buffer_size), &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(glBufferData(gl_type, narrow_cast<GLsizei>(buffer_size), &m_buffer.front(),
                                       this->_to_gl_usage(this->get_usage_hint())));
            m_server_size = buffer_size;
            this->_log_buffer_size(buffer_size);
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
