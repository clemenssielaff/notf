#pragma once

#include <tuple>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "common/vector3.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/shader.hpp"
#include "utils/static_string.hpp"

//**********************************************************************************************************************

namespace notf {

namespace detail {

template <typename TUPLE, std::size_t... I>
decltype(auto) extract_trait_types_impl(const TUPLE&, std::index_sequence<I...>)
{
    return std::tuple<typename std::tuple_element<I, TUPLE>::type::type...>{};
}

/** Extracts the value types from a variadic list of traits. */
template <typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
decltype(auto) extract_trait_types(const std::tuple<Ts...>& tuple)
{
    return extract_trait_types_impl(tuple, Indices{});
}

} // namespace detail

//**********************************************************************************************************************

/** An overengineered VertexBuffer class using metaprogramming.
 *
 * Example usage:
 *
 *     namespace detail {
 *
 *     struct VertexPositionTrait {
 *         constexpr static StaticString name = "vPos";     // matches the name of an attribute in the shader
 *         using type                         = Vector3f;   // attribute value type
 *     };
 *
 *     struct VertexColorTrait {
 *         constexpr static StaticString name = "vColor";
 *         using type                         = Color;
 *     };
 *
 *     } // namespace detail
 *
 *     using VertexLayout = VertexBuffer<VertexPositionTrait, VertexColorTrait>;
 *     std::vector<VertexLayout::Vertex> vertices;
 *     vertices.emplace_back(Vector3f(-1, -1, 0), Vector3f(1, 0, 0));
 *     vertices.emplace_back(Vector3f(0, 1, 0), Vector3f(0, 1, 0));
 *     vertices.emplace_back(Vector3f(1, -1, 0), Vector3f(0, 0, 1));
 *
 *     VertexLayout vertex_buffer(std::move(vertices));
 *
 *     ...
 *
 *     vertex_buffer.init(shader); // make sure that a VAO is bound at this point, otherwise this will throw
 *
 * and that's it.
 */
template <typename... Ts>
class VertexBuffer {

public: // types ******************************************************************************************************/
    /** Typdef for a tuple containing the trait types in order. */
    using Vertex = decltype(detail::extract_trait_types(std::tuple<Ts...>{}));

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer)

    VertexBuffer(std::vector<Vertex>&& vertices)
        : m_buffer_id(0)
        , m_vertices(std::move(vertices))
    {
        glGenBuffers(1, &m_buffer_id);
        if (!m_buffer_id) {
            throw_runtime_error("Failed to alocate VertexBuffer");
        }
    }

    /** Destructor. */
    ~VertexBuffer()
    {
        glDeleteBuffers(1, &m_buffer_id);
    }

    /** Initializes the VertexBuffer.
     * @throws std::runtime_error   If no VAO object is currently bound.
     */
    void init(ShaderConstPtr shader)
    {
        { // make sure there is a bound VAO
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            if (!current_vao) {
                throw_runtime_error("Cannot initialize a VertexBuffer without a bound VertexArrayObject");
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_buffer_id);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);

        if (!m_vertices.empty()) {
            _init_buffer<0, Ts...>(std::move(shader));
        }

        check_gl_error();
    }

private: // methods ***************************************************************************************************/
    /** Recursively define each attribute from the class' traits. */
    template <size_t INDEX, typename FIRST, typename SECOND, typename... REST>
    void _init_buffer(ShaderConstPtr shader)
    {
        _define_attribute<INDEX, FIRST>(shader);
        _init_buffer<INDEX + 1, SECOND, REST...>(std::move(shader));
    }

    /** Recursion breaker. */
    template <size_t INDEX, typename LAST>
    void _init_buffer(ShaderConstPtr shader)
    {
        _define_attribute<INDEX, LAST>(shader);
    }

    /** Defines a single attribute. */
    template <size_t INDEX, typename ATTRIBUTE>
    void _define_attribute(ShaderConstPtr shader)
    {
        // we cannot be sure that the tuple stores its fields in order :/
        // therefore we have to discover the offset for each element
        const auto offset = reinterpret_cast<std::intptr_t>(&std::get<INDEX>(m_vertices[0]))
            - reinterpret_cast<std::intptr_t>(&m_vertices[0]);
        assert(offset >= 0);

        const GLuint attribute_id = shader->attribute(std::string(ATTRIBUTE::name.ptr, ATTRIBUTE::name.size));
        glEnableVertexAttribArray(attribute_id);
        glVertexAttribPointer(
            attribute_id, sizeof(typename ATTRIBUTE::type) / sizeof(float), GL_FLOAT, /*normalized*/ GL_FALSE,
            static_cast<GLsizei>(sizeof(Vertex)), gl_buffer_offset(static_cast<size_t>(offset)));
    }

private: // fields ****************************************************************************************************/
    /** OpenGL handle of the array buffer. */
    GLuint m_buffer_id;

    /** Content of the array buffer. */
    std::vector<Vertex> m_vertices;
};

} // namespace notf
