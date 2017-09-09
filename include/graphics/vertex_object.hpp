#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/vertex_buffer.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(class, Shader)

namespace detail {

/** Finds the smallest OpenGL unsigned integer type capable of representing the given value.
 * Is either GLubyte, GLushort or GLuint.
 */
template <signed long long VALUE>
struct gl_smallest_unsigned_type {
    static_assert(VALUE >= 0, "Index buffer index cannot be less than zero");
    static_assert(VALUE <= std::numeric_limits<GLuint>::max(), "Index buffer index too large (must fit into a Gluint)");
    using type = typename std::conditional_t<VALUE <= std::numeric_limits<GLubyte>::max(),
                                             GLubyte,
                                             typename std::conditional_t<VALUE <= std::numeric_limits<GLushort>::max(),
                                                                         GLushort,
                                                                         GLuint>>;
};
}

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** IndexBuffer baseclass, so other objects can hold pointers to any type of IndexBuffer.
 */
class IndexBufferType {

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(IndexBufferType)

    /** Destructor. */
    virtual ~IndexBufferType();

    /** Initializes the IndexBuffer.
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

protected: // methods *************************************************************************************************/
    /** Protected Constructor. */
    IndexBufferType()
        : m_vbo_id(0)
        , m_type(0)
        , m_size(0)
    {
    }

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
template <typename INDEX_TYPE>
class IndexBuffer : public IndexBufferType {

public: // type *******************************************************************************************************/
    /** Value type of the indices. */
    using index_t = INDEX_TYPE;

public: // methods ***************************************************************************************************/
    /** Constructor. */
    IndexBuffer(std::vector<index_t>&& indices)
        : IndexBufferType()
        , m_indices(std::move(indices))
    {
        m_type = _type();
        m_size = static_cast<GLsizei>(m_indices.size());
    }

    virtual void init() override
    {
        if (m_vbo_id) {
            return;
        }

        glGenBuffers(1, &m_vbo_id);
        if (!m_vbo_id) {
            throw_runtime_error("Failed to alocate IndexBuffer");
        }

        { // make sure there is a bound VAO
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            if (!current_vao) {
                throw_runtime_error("Cannot initialize an IndexBuffer without a bound VAO");
            }
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(index_t), &m_indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        check_gl_error();
    }

private: // methods ***************************************************************************************************/
    template <bool enable = true>
    typename std::enable_if<std::is_same<index_t, GLushort>::value && enable, GLenum>::type
    _type() const { return GL_UNSIGNED_SHORT; }

    template <bool enable = true>
    typename std::enable_if<std::is_same<index_t, GLubyte>::value && enable, GLenum>::type
    _type() const { return GL_UNSIGNED_BYTE; }

    template <bool enable = true>
    typename std::enable_if<std::is_same<index_t, GLuint>::value && enable, GLenum>::type
    _type() const { return GL_UNSIGNED_INT; }

private: // fields ****************************************************************************************************/
    /** Index data. */
    std::vector<index_t> m_indices;
};

// factory ************************************************************************************************************/

/** Creates an IndexBuffer object containing the given indices in their smalles representable form.
 * Passing an index less than zero or larger than a GLuint can contain raises an error during compilation.
 */
template <size_t... indices>
decltype(auto) create_index_buffer()
{
    using value_t = typename detail::gl_smallest_unsigned_type<std::max<size_t>({indices...})>::type;
    return std::make_unique<IndexBuffer<value_t>>(std::vector<value_t>{indices...});
}

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/**
 *
 */
class VertexObject {

public: // type *******************************************************************************************************/
    enum class RenderMode : unsigned char {
        POINTS,
        LINE_STRIP,
        LINE_LOOP,
        LINES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
        TRIANGLES,
    };

public: // methods **************************************************************************************************/
    VertexObject(ShaderPtr shader, VertexBufferTypePtr vertices, std::unique_ptr<IndexBufferType> indices = {})
        : m_vao_id(0)
        , m_mode(GL_TRIANGLES)
        , m_shader(std::move(shader))
        , m_vertices(std::move(vertices))
        , m_indices(std::move(indices))
    {
        glGenVertexArrays(1, &m_vao_id);
        if (!m_vao_id) {
            throw_runtime_error("Failed to alocate VertexObject");
        }

        glBindVertexArray(m_vao_id);
        m_vertices->init(m_shader);
        if (m_indices) {
            m_indices->init();
        }
    }

    void render() const
    {
        glBindVertexArray(m_vao_id);
        if (m_indices) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices->id());
            glDrawElements(m_mode, m_indices->size(), m_indices->type(), 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        else {
            glDrawArrays(m_mode, 0, m_vertices->size());
        }
        glBindVertexArray(0);
    }

private: // fields ****************************************************************************************************/
    /** OpenGL handle of the vertex array object. */
    GLuint m_vao_id;

    GLenum m_mode;

    /** Shader used to draw this vertex object. */
    ShaderPtr m_shader;

    /** Vertex buffer to draw this object from. */
    VertexBufferTypePtr m_vertices;

    /** Index buffer, laeve empty to draw vertices in the order they appear in the VertexBuffer. */
    std::unique_ptr<IndexBufferType> m_indices;
};

} // namespace notf
