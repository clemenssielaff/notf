#pragma once

#include <assert.h>
#include <iostream>
#include <tuple>
#include <vector>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "core/opengl.hpp"
#include "graphics/engine/gl_errors.hpp"
#include "graphics/engine/gl_utils.hpp"
#include "graphics/engine/shader.hpp"

//**********************************************************************************************************************

namespace notf {

namespace detail {

class PrefabFactoryImpl;

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

    /** Vertex normal vector. */
    struct Normal {
    };

    /** Vertex color. */
    struct Color {
    };

    /** Texture coordinate. */
    struct TexCoord {
    };

    /** Catch-all for other attribute kinds.
     * Does not impose any restrictions on the Trait::type.
     */
    struct Other {
    };
};

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** VertexArray baseclass, so other objects can hold pointers to any type of VertexArray.
 */
class VertexArrayType {

public: // types ******************************************************************************************************/
    /** Arguments for the vertex array. */
    struct Args {

        /** The expected usage of the data.
         * Must be one of:
         * GL_STREAM_DRAW    GL_STATIC_DRAW    GL_DYNAMIC_DRAW
         * GL_STREAM_READ    GL_STATIC_READ    GL_DYNAMIC_READ
         * GL_STREAM_COPY    GL_STATIC_COPY    GL_DYNAMIC_COPY
         */
        GLenum usage = GL_STATIC_DRAW;

        /** Whether this array is per-vertex or per-instance. */
        bool per_instance = false;
    };

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
    VertexArrayType(Args&& args)
        : m_args(std::move(args))
        , m_vbo_id(0)
        , m_size(0)
    {
    }

    /** Tries to find an attribute in the shader.
     * Logs a warning message, if the attribute could not be found.
     * @param attribute_name    Name of the attribute to find.
     * @return                  Attribute ID or INVALID_ID if the attribute was not found.
     */
    GLuint _get_shader_attribute(const Shader* shader, const std::string attribute_name);

protected: // fields **************************************************************************************************/
    /** Arguments used to initialize the vertex array. */
    const Args m_args;

    /** OpenGL handle of the vertex buffer. */
    GLuint m_vbo_id;

    /** Number of elements in the array. */
    GLsizei m_size;

protected: // static fields *******************************************************************************************/
    /** Invalid attribute ID. */
    static constexpr GLuint INVALID_ID = std::numeric_limits<GLuint>::max();
};

//*********************************************************************************************************************/
//**********************************************************************************************************************

/** The Vertex array manages an array of vertex attributes.
 * The array's layout is defined at compile-time using traits.
 *
 * Example usage:
 *
 *     namespace detail {
 *
 *     struct VertexPositionTrait {
 *         StaticString name             = "vPos";                  // matches the name of an attribute in the shader
 *         using type                    = float;                   // type to store the value (is converted to OpenGL)
 *         using kind                    = AttributeKind::Position; // attribute value kind (used by GeometryFactory)
 *         static constexpr size_t count = 4;                       // number of components in the value (3 for vec3...)
 *     };
 *
 *     struct VertexColorTrait {
 *         StaticString name             = "vColor";
 *         using type                    = float;
 *         using kind                    = AttributeKind::Color;
 *         static constexpr size_t count = 4;
 *     };
 *
 *     } // namespace detail
 *
 *     using VertexLayout = VertexArray<VertexPositionTrait, VertexColorTrait>;
 */
template <typename... Ts>
class VertexArray : public VertexArrayType {

    template <typename>
    friend class PrefabFactory;

public: // types ******************************************************************************************************/
    /** Traits defining the array layout. */
    using Traits = std::tuple<Ts...>;

    /** Typdef for a tuple containing the trait types in order. */
    using Vertex = decltype(detail::extract_trait_types(Traits{}));

public: // methods ****************************************************************************************************/
    /** Constructor. */
    VertexArray(Args&& args = {})
        : VertexArrayType(std::forward<Args>(args))
        , m_vertices()
        , m_buffer_size(0)
    {
    }

    /** Initializes the VertexArray.
     * @param shader                Shader to initialize the attributes.
     * @throws std::runtime_error   If the VBO could not be allocated.
     * @throws std::runtime_error   If no VAO object is currently bound.
     */
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

        m_size = static_cast<GLsizei>(m_vertices.size());

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, m_size * sizeof(Vertex), &m_vertices[0], m_args.usage);
        _init_array<0, Ts...>(std::move(shader));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check_gl_error();

        m_vertices.clear();
        m_vertices.shrink_to_fit();
    }

    /** Updates the data in the vertex array.
     * If you regularly want to update the data, make sure you pass an appropriate `usage` hint in the arguments.
     * @param data  New data to upload.
     * @throws std::runtime_error   If the VertexArray is not yet initialized.
     * @throws std::runtime_error   If no VAO object is currently bound.
     */
    void update(std::vector<Vertex>&& data)
    {
        if (!m_vbo_id) {
            throw_runtime_error("Cannot update an unitialized VertexArray");
        }

        { // make sure there is a bound VAO
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            if (!current_vao) {
                throw_runtime_error("Cannot update a VertexArray without a bound VAO");
            }
        }

        // update vertex array
        std::swap(m_vertices, data);
        m_size = static_cast<GLsizei>(m_vertices.size());

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        if (m_size <= m_buffer_size) {
            // if the new data is smaller or of equal size than the last one, we can do a minimal update
            glBufferSubData(GL_ARRAY_BUFFER, /*offset*/ 0, m_size * sizeof(Vertex), &m_vertices[0]);
        }
        else {
            // otherwise we have to do a full update
            glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], m_args.usage);
        }
        m_buffer_size = std::max(m_buffer_size, m_size);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        check_gl_error();

        m_vertices.clear();
        m_vertices.shrink_to_fit();
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

        // find the attribute id
        const GLuint attribute_id = _get_shader_attribute(shader.get(), ATTRIBUTE::name);
        if (attribute_id == INVALID_ID) {
            return;
        }

        // not all attribute types fit into a single OpenGL ES attribute
        // larger types are stored in consecutive attribute locations
        for (size_t multi = 0; multi < (ATTRIBUTE::count / 4) + (ATTRIBUTE::count % 4 ? 1 : 0); ++multi) {

            const GLint size = std::min(4, static_cast<int>(ATTRIBUTE::count) - static_cast<int>((multi * 4)));
            assert(size >= 1 && size <= 4);

            // link the location in the array to the shader's attribute
            glEnableVertexAttribArray(attribute_id + multi);
            glVertexAttribPointer(
                attribute_id + multi,
                size,
                to_gl_type(typename ATTRIBUTE::type{}),
                /*normalized*/ GL_FALSE,
                static_cast<GLsizei>(sizeof(Vertex)),
                gl_buffer_offset(static_cast<size_t>(offset + (multi * 4 * sizeof(GLfloat)))));

            // define the attribute as an instance attribute
            if (m_args.per_instance) {
                glVertexAttribDivisor(attribute_id + multi, 1);
            }
        }
    }

private: // fields ****************************************************************************************************/
    /** Vertices stored in the array. */
    std::vector<Vertex> m_vertices;

    /** Size in bytes of the buffer allocated on the server. */
    GLsizei m_buffer_size;
};

} // namespace notf
