#pragma once

#include <limits>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "common/numeric.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/opengl.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

/// Finds the smallest OpenGL unsigned integer type capable of representing the given value.
/// Is either GLubyte, GLushort or GLuint.
template<signed long long VALUE>
struct gl_smallest_unsigned_type {
    static_assert(VALUE >= 0, "Index buffer index cannot be less than zero");
    static_assert(VALUE <= max_value<GLuint>(), "Index buffer index too large (must fit into a Gluint)");
    using type =
        typename std::conditional_t<VALUE <= max_value<GLubyte>(), GLubyte,
                                    typename std::conditional_t<VALUE <= max_value<GLushort>(), GLushort, GLuint>>;
};

} // namespace detail

// ================================================================================================================== //

/// IndexArray baseclass, so other objects can hold pointers to any type of IndexArray.
class IndexArrayType {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Arguments for the index array.
    struct Args {

        /// The expected usage of the data.
        GLUsage usage = GLUsage::DEFAULT;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @throws runtime_error   If there is no OpenGL context.
    IndexArrayType(Args&& args) : m_args(std::move(args)), m_vbo_id(0), m_type(0), m_size(0) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(IndexArrayType);

    /// Destructor.
    virtual ~IndexArrayType();

    /// OpenGL handle of the index buffer.
    GLuint get_id() const { return m_vbo_id; }

    /// OpenGL enum value of the type of indices contained in the buffer.
    GLenum get_type() const { return m_type; }

    /// Number of elements to draw.
    GLsizei get_size() const { return m_size; }

    /// Checks whether the array is empty.
    bool is_empty() const { return m_size == 0; }

    /// The restart index of the index buffer type.
    virtual GLuint get_restart_index() const = 0;

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Arguments used to initialize the index array.
    const Args m_args;

    /// OpenGL handle of the index buffer.
    GLuint m_vbo_id;

    /// Type of the indices contained in the buffer.
    GLenum m_type;

    /// Number of elements to draw.
    GLsizei m_size;
};

// ================================================================================================================== //

/// Abstraction of an OpenGL index buffer.
template<typename INDEX_TYPE>
class IndexArray : public IndexArrayType {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Value type of the indices.
    using index_t = INDEX_TYPE;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @throws runtime_error   If there is no OpenGL context.
    IndexArray(Args&& args = {}) : IndexArrayType(std::forward<Args>(args)), m_indices(), m_buffer_size(0) {}

    /// Write-access to the index buffer.
    /// Note that you need to `init()` (if it is the first time) or `update()` to apply the contents of the buffer.
    std::vector<index_t>& buffer() { return m_indices; }

    /// Initializes the IndexArray.
    /// @throws runtime_error   If the VBO could not be allocated.
    /// @throws runtime_error   If no VAO is currently bound.
    void init()
    {
        { // make sure there is a bound VAO
            GLint current_vao = 0;
            notf_check_gl(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(runtime_error, "Cannot initialize an IndexArray without a bound VAO");
            }
        }

        if (m_vbo_id) {
            return _update();
        }

        notf_check_gl(glGenBuffers(1, &m_vbo_id));
        if (!m_vbo_id) {
            NOTF_THROW(runtime_error, "Failed to allocate IndexArray");
        }

        m_type = to_gl_type(index_t());
        m_size = static_cast<GLsizei>(m_indices.size());

        notf_check_gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id));
        notf_check_gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(index_t), &m_indices[0],
                                   get_gl_usage(m_args.usage)));
        // keep the buffer bound as it is stored in the VAO

        m_indices.clear();
        m_indices.shrink_to_fit();
    }

private:
    /// Updates the data in the index array.
    /// If you regularly want to update the data, make sure you pass an appropriate `usage` hint in the arguments.
    void _update()
    {
        m_size = static_cast<GLsizei>(m_indices.size());

        notf_check_gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id));
        if (m_size <= m_buffer_size) {
            // if the new data is smaller or of equal size than the last one, we can do a minimal update
            notf_check_gl(
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, /*offset = */ 0, m_size * sizeof(index_t), &m_indices[0]));
        }
        else {
            // otherwise we have to do a full update
            notf_check_gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(index_t), &m_indices[0],
                                       get_gl_usage(m_args.usage)));
        }
        // keep the buffer bound as it is stored in the VAO

        m_buffer_size = std::max(m_buffer_size, m_size);

        m_indices.clear();
        // do not shrink to size, if you call `update` once you are likely to call it again
    }

    /// Value to use as the restart index.
    virtual GLuint get_restart_index() const override { return max_value<GLuint>(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Index data.
    std::vector<index_t> m_indices;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_buffer_size;
};

// ================================================================================================================== //

/// Creates an IndexArray object containing the given indices in their smalles representable form.
/// Passing an index less than zero or larger than a GLuint can contain raises an error during compilation.
/// The returned IndexBuffer is not yet initialized.
template<size_t... INDICES>
decltype(auto) create_index_buffer()
{
    using value_t = typename detail::gl_smallest_unsigned_type<std::max<size_t>({INDICES...})>::type;
    auto result = std::make_unique<IndexArray<value_t>>();
    result->buffer() = std::vector<value_t>{INDICES...};
    return result;
}

NOTF_CLOSE_NAMESPACE
