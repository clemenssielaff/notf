#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"

namespace notf {

namespace detail {

/** Finds the smallest OpenGL unsigned integer type capable of representing the given value.
 * Is either GLubyte, GLushort or GLuint.
 */
template<signed long long VALUE>
struct gl_smallest_unsigned_type {
    static_assert(VALUE >= 0, "Index buffer index cannot be less than zero");
    static_assert(VALUE <= std::numeric_limits<GLuint>::max(), "Index buffer index too large (must fit into a Gluint)");
    using type = typename std::conditional_t<
        VALUE <= std::numeric_limits<GLubyte>::max(), GLubyte,
        typename std::conditional_t<VALUE <= std::numeric_limits<GLushort>::max(), GLushort, GLuint>>;
};

} // namespace detail

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** IndexArray baseclass, so other objects can hold pointers to any type of IndexArray.
 */
class IndexArrayType {

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(IndexArrayType)

    /** Destructor. */
    virtual ~IndexArrayType();

    /** Initializes the IndexArray.
     * @throws std::runtime_error   If the VBO could not be allocated.
     * @throws std::runtime_error   If no VAO object is currently bound.
     */
    virtual void init() = 0;

    /** OpenGL handle of the index buffer. */
    GLuint id() const { return m_vbo_id; }

    /** Type of the indices contained in the buffer. */
    GLenum type() const { return m_type; }

    /** Number of elements to draw. */
    GLsizei size() const { return m_size; }

    /** The restart index of the index buffer type. */
    virtual GLuint restart_index() const = 0;

protected: // methods *************************************************************************************************/
    /** Protected Constructor. */
    IndexArrayType() : m_vbo_id(0), m_type(0), m_size(0) {}

protected: // fields **************************************************************************************************/
    /** OpenGL handle of the index buffer. */
    GLuint m_vbo_id;

    /** Type of the indices contained in the buffer. */
    GLenum m_type;

    /** Number of elements to draw. */
    GLsizei m_size;
};

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** Abstracts an OpenGL index buffer.
 *
 */
template<typename INDEX_TYPE>
class IndexArray : public IndexArrayType {

    template<size_t... indices>
    friend decltype(auto) create_index_buffer();

    template<typename>
    friend class PrefabFactory;

public: // type *******************************************************************************************************/
    /** Value type of the indices. */
    using index_t = INDEX_TYPE;

public: // methods ***************************************************************************************************/
    /** Constructor. */
    IndexArray() : IndexArrayType(), m_indices() {}

    virtual void init() override
    {
        if (m_vbo_id) {
            return;
        }

        glGenBuffers(1, &m_vbo_id);
        if (!m_vbo_id) {
            throw_runtime_error("Failed to allocate IndexArray");
        }

        m_type = to_gl_type(index_t());
        m_size = static_cast<GLsizei>(m_indices.size());

        { // make sure there is a bound VAO
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            if (!current_vao) {
                throw_runtime_error("Cannot initialize an IndexArray without a bound VAO");
            }
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(index_t), &m_indices[0], GL_STATIC_DRAW);
        // keep the buffer bound as it is stored in the VAO
        check_gl_error();
    }

    virtual GLuint restart_index() const override { return static_cast<GLuint>(std::numeric_limits<index_t>::max()); }

private: // fields ****************************************************************************************************/
    /** Index data. */
    std::vector<index_t> m_indices;
};

// factory ************************************************************************************************************/

/** Creates an IndexArray object containing the given indices in their smalles representable form.
 * Passing an index less than zero or larger than a GLuint can contain raises an error during compilation.
 */
template<size_t... indices>
decltype(auto) create_index_buffer()
{
    using value_t     = typename detail::gl_smallest_unsigned_type<std::max<size_t>({indices...})>::type;
    auto result       = std::make_unique<IndexArray<value_t>>();
    result->m_indices = std::vector<value_t>{indices...};
    return result;
}

} // namespace notf
