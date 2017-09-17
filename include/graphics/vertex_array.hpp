#pragma once

#include <assert.h>
#include <iostream>
#include <tuple>
#include <vector>

#include "common/exception.hpp"
#include "common/half.hpp"
#include "common/log.hpp"
#include "common/meta.hpp"
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
    return std::tuple<std::array<
        typename std::tuple_element<I, TUPLE>::type::type,
        std::tuple_element<I, TUPLE>::type::count>...>{};
}

/** Extracts the value types from a variadic list of traits. */
template <typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
decltype(auto) extract_trait_types(const std::tuple<Ts...>& tuple)
{
    return extract_trait_types_impl(tuple, Indices{});
}

} // namespace detail

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** Definitions used to identify VertexArray traits to the Geometry factory.
 * Used to tell the GeometryFactory how to construct a VertexArray<Traits...>::Vertex instance.
 * Using an AttributeKind other than AttributeKind::Other determines the Trait's type as well.
 */
struct AttributeKind {

    /** Vertex position in model space. */
    struct Position {
    };

    /** Texture coordinate. */
    struct TexCoord {
    };

    /** Vertex normal vector. */
    struct Normal {
    };

    /** Vertex color. */
    struct Color {
    };

    /** Catch-all for other attribute kinds.
     * Does not impose any restrictions on the Trait::type.
     */
    struct Other {
    };
};

//*********************************************************************************************************************/
//*********************************************************************************************************************/

DEFINE_SHARED_POINTERS(class, VertexArrayType);

/** VertexArray baseclass, so other objects can hold pointers to any type of VertexArray.
 */
class VertexArrayType {

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(VertexArrayType)

    /** Destructor. */
    virtual ~VertexArrayType();

    /** Initializes the VertexArray.
     * @throws std::runtime_error   If the VBO could not be allocated.
     * @throws std::runtime_error   If no VAO object is currently bound.
     */
    virtual void init(ShaderConstPtr shader) = 0;

    /** OpenGL handle of the vertex buffer. */
    GLuint id() const { return m_vbo_id; }

    /** Number of elements in the array. */
    GLsizei size() const { return m_size; }

protected: // methods *************************************************************************************************/
    /** Protected Constructor. */
    VertexArrayType()
        : m_vbo_id(0)
        , m_size(0)
    {
    }

protected: // fields **************************************************************************************************/
    /** OpenGL handle of the vertex buffer. */
    GLuint m_vbo_id;

    /** Number of elements in the array. */
    GLsizei m_size;
};

//*********************************************************************************************************************/
//**********************************************************************************************************************

// TODO: update VertexArray docstring

/** Abstracts an OpenGL VBO.
 *
 * Example usage:
 *
 *     namespace detail {
 *
 *     struct VertexPositionTrait {
 *         StaticString name                = "vPos";                  // matches the name of an attribute in the shader
 *         using type                       = Vector3f;                // attribute value type
 *         constexpr static GLenum gltype   = GL_FLOAT;                // OpenGL type to store the value
 *         using kind                       = AttributeKind::Position; // attribute value kind (used by GeometryFactory)
 *     };
 *
 *     struct VertexColorTrait {
 *         StaticString name                = "vColor";
 *         using type                       = Color;
 *         constexpr static GLenum gltype   = GL_HALF_FLOAT;
 *         using kind                       = AttributeKind::Color;
 *     };
 *
 *     } // namespace detail
 *
 *     using VertexLayout = VertexArray<VertexPositionTrait, VertexColorTrait>;
 *     std::vector<VertexLayout::Vertex> vertices;
 *     vertices.emplace_back(Vector3f(-1, -1, 0), Vector3f(1, 0, 0));
 *     vertices.emplace_back(Vector3f(0, 1, 0), Vector3f(0, 1, 0));
 *     vertices.emplace_back(Vector3f(1, -1, 0), Vector3f(0, 0, 1));
 *
 *     VertexLayout vertex_array(std::move(vertices));
 *
 *     ...
 *
 *     vertex_array.init(shader); // make sure that a VAO is bound at this point, otherwise this will throw
 *
 * and that's it.
 */
template <typename... Ts>
class VertexArray : public VertexArrayType {

public: // types ******************************************************************************************************/
    /** Traits defining the array layout. */
    using Traits = std::tuple<Ts...>;

    /** Typdef for a tuple containing the trait types in order. */
    using Vertex = decltype(detail::extract_trait_types(Traits{}));

public: // methods ****************************************************************************************************/
    /** Constructor. */
    VertexArray(std::vector<Vertex>&& vertices)
        : VertexArrayType()
        , m_vertices(std::move(vertices))
    {
        m_size = static_cast<GLsizei>(m_vertices.size());
    }

    virtual void init(ShaderConstPtr shader) override
    {
        if (m_vbo_id) {
            return;
        }

        glGenBuffers(1, &m_vbo_id);
        if (!m_vbo_id) {
            throw_runtime_error("Failed to allocate VertexArray");
        }

        { // make sure there is a bound VAO
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            if (!current_vao) {
                throw_runtime_error("Cannot initialize a VertexArray without a bound VAO");
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);
        if (!m_vertices.empty()) {
            _init_array<0, Ts...>(std::move(shader));
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        check_gl_error();
    }

private: // methods ***************************************************************************************************/
    /** Recursively define each attribute from the class' traits. */
    template <size_t INDEX, typename FIRST, typename SECOND, typename... REST>
    void _init_array(ShaderConstPtr shader)
    {
        _define_attribute<INDEX, FIRST>(shader);
        _init_array<INDEX + 1, SECOND, REST...>(std::move(shader));
    }

    /** Recursion breaker. */
    template <size_t INDEX, typename LAST>
    void _init_array(ShaderConstPtr shader)
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

        const std::string attribute_name = std::string(ATTRIBUTE::name.ptr, ATTRIBUTE::name.size);
        GLuint attribute_id;
        try {
            attribute_id = shader->attribute(attribute_name);
        } catch (const std::runtime_error&) {
            log_warning << "Ignoring unknown attribute: \"" << attribute_name << "\"";
            return;
        }

        glEnableVertexAttribArray(attribute_id);
        glVertexAttribPointer(
            attribute_id,
            ATTRIBUTE::count,
            to_gl_type(typename ATTRIBUTE::type{}),
            /*normalized*/ GL_FALSE,
            static_cast<GLsizei>(sizeof(Vertex)),
            gl_buffer_offset(static_cast<size_t>(offset)));
    }

public: // fields ****************************************************************************************************/
    /** Vertices stored in the array. */
    std::vector<Vertex> m_vertices;
};

} // namespace notf
