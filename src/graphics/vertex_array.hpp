#pragma once

#include <limits>

#include "common/assert.hpp"
#include "common/exception.hpp"
#include "common/meta.hpp"
#include "common/numeric.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/opengl.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace detail {

class PrefabFactoryImpl;

template<typename TUPLE, std::size_t... I>
constexpr decltype(auto) extract_trait_types_impl(const TUPLE&, std::index_sequence<I...>)
{
    return std::tuple<typename std::tuple_element<I, TUPLE>::type::type...>{};
}

/// Extracts the value types from a variadic list of traits.
template<typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
constexpr decltype(auto) extract_trait_types(const std::tuple<Ts...>& tuple)
{
    return extract_trait_types_impl(tuple, Indices{});
}

} // namespace detail

// ================================================================================================================== //

/// Definitions used to identify VertexArray traits to the Geometry factory.
/// Used to tell the GeometryFactory how to construct a VertexArray<Traits...>::Vertex instance.
struct AttributeKind {

    /// Vertex position in model space.
    struct Position {};

    /// Vertex normal vector.
    struct Normal {};

    /// Vertex color.
    struct Color {};

    /// Texture coordinate.
    struct TexCoord {};

    /// Catch-all for other attribute kinds.
    /// Does not impose any restrictions on the Trait::type.
    struct Other {};
};

// ================================================================================================================== //

/// Base of all attribute kinds.
/// The base holds defaults for all trait types - some of which must be overwritten in subclasses in order to
/// pass `is_attribute_trait`.
struct AttributeTrait {

    /// Location of the attribute in the shader.
    constexpr static uint location = max_value<GLuint>(); // INVALID DEFAULT

    /// Type used to store the trait value.
    using type = void; // INVALID DEFAULT

    /// Whether the value type is normalized or not.
    /// See https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribPointer.xhtml
    constexpr static bool normalized = false;

    /// Attribute kind, is used by the GeometryFactory to identify the trait.
    using kind = AttributeKind::Other;
};

/// Trait checker.
/// Can be used to statically assert whether a given type is indeed an AttributeTrait.
template<class T, class = void>
struct is_attribute_trait : std::false_type { // false for all T by default
};
template<class T>
struct is_attribute_trait<
    T, std::void_t<
           // does the type contain a `uint` that has a valid value?
           std::enable_if_t<std::is_same<decltype(T::location), const uint>::value>,
           std::enable_if_t<T::location != max_value<GLuint>()>,

           // does the type contain a `bool` named `normalized` ?
           std::enable_if_t<std::is_same<decltype(T::normalized), const bool>::value>,

           // does type type contain a type named `type`?
           typename T::type,

           // does type type contain a type named `kind` that is a subtype of `AttributeKind`?
           typename T::kind,
           std::enable_if_t<is_one_of<typename T::kind, AttributeKind::Position, AttributeKind::Normal,
                                      AttributeKind::Color, AttributeKind::TexCoord, AttributeKind::Other>::value>>>
    : std::true_type {};

namespace detail {

template<typename TUPLE, std::size_t... I>
constexpr bool is_trait_tuple_impl(const TUPLE&, std::index_sequence<I...>)
{
    return std::conjunction<is_attribute_trait<typename std::tuple_element<I, TUPLE>::type>...>::value;
}

} // namespace detail

/// Checks whether a tuple contains only Attribute traits.
/// @param tuple    Tuple to check.
template<typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
constexpr inline bool is_trait_tuple(const std::tuple<Ts...>& tuple)
{
    return detail::is_trait_tuple_impl(tuple, Indices{});
}

// ================================================================================================================== //

/// VertexArray baseclass, so other objects can hold pointers to any type of VertexArray.
class VertexArrayType {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Arguments for the vertex array.
    struct Args {
        /// The expected usage of the data.
        GLUsage usage = GLUsage::STATIC_DRAW;

        /// Whether attributes in this array are applied per-vertex or per-instance.
        bool per_instance = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @throws runtime_error   If there is no OpenGL context.
    VertexArrayType(Args&& args) : m_args(std::move(args))
    {
        if (!gl_is_initialized()) {
            NOTF_THROW(runtime_error, "Cannot create a VertexArray without an OpenGL context");
        }
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(VertexArrayType);

    /// Destructor.
    virtual ~VertexArrayType();

    /// OpenGL handle of the vertex buffer.
    GLuint get_id() const { return m_vbo_id; }

    /// Number of elements in the array.
    GLsizei get_size() const { return m_size; }

    /// Checks whether the array is empty.
    bool is_empty() const { return m_size == 0; }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Arguments used to initialize the vertex array.
    const Args m_args;

    /// OpenGL handle of the vertex buffer.
    GLuint m_vbo_id = 0;

    /// Number of elements in the array.
    GLsizei m_size = 0;

    /// Invalid attribute ID.
    static constexpr GLuint INVALID_ID = max_value<GLuint>();
};

// ================================================================================================================== //

/// The Vertex array manages an array of vertex attributes.
/// The array's layout is defined at compile-time using traits.
///
/// Example usage:
///
///     namespace detail {
///
///     struct VertexPositionTrait : public AttributeTrait {
///         constexpr static uint location = 0;
///         using type                     = Vector2f;
///         using kind                     = AttributeKind::Position;
///     };
///
///     struct VertexColorTrait : public AttributeTrait {
///         constexpr static uint location = 1;
///         using type                     = Vector4h;
///         using kind                     = AttributeKind::Color;
///     };
///
///     } // namespace detail
///
///     using VertexLayout = VertexArray<VertexPositionTrait, VertexColorTrait>;
///
template<typename... Ts>
class VertexArray : public VertexArrayType {

    template<typename>
    friend class PrefabFactory;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Traits defining the array layout.
    using Traits = std::tuple<Ts...>;

    /// A tuple containing one array for each trait.
    using Vertex = decltype(detail::extract_trait_types(Traits{}));

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param args             VertexArray arguments (defaults to default constructed argument struct).
    /// @throws runtime_error   If there is no OpenGL context.
    VertexArray(Args&& args = {}) : VertexArrayType(std::forward<Args>(args))
    {
        static_assert(std::tuple_size<Traits>::value > 0, "A VertexArray must contain at least one Attribute");
        static_assert(is_trait_tuple(Traits{}), "Template arguments to VertexArray must only contain valid "
                                                "AttributeTrait types.");
    }

    /// Write-access to the vertex buffer.
    /// Note that you need to `init()` (if it is the first time) or `update()` to apply the contents of the buffer.
    std::vector<Vertex>& get_buffer() { return m_vertices; }

    /// Initializes the VertexArray with the current contents of `m_vertices`.
    /// @throws runtime_error   If the VBO could not be allocated.
    /// @throws runtime_error   If no VAO is currently bound.
    void init()
    {
        { // make sure there is a bound VAO
            GLint current_vao = 0;
            notf_check_gl(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(runtime_error, "Cannot initialize a VertexArray without a bound VAO");
            }
        }

        if (m_vbo_id) {
            return _update();
        }

        notf_check_gl(glGenBuffers(1, &m_vbo_id));
        if (!m_vbo_id) {
            NOTF_THROW(runtime_error, "Failed to allocate VertexArray");
        }

        m_size = static_cast<GLsizei>(m_vertices.size());

        notf_check_gl(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
        notf_check_gl(
            glBufferData(GL_ARRAY_BUFFER, m_size * sizeof(Vertex), &m_vertices[0], get_gl_usage(m_args.usage)));
        _init_array<0, Ts...>();
        notf_check_gl(glBindBuffer(GL_ARRAY_BUFFER, 0));

        m_vertices.clear();
        m_vertices.shrink_to_fit();
    }

private:
    /// Updates the data in the vertex array.
    /// If you regularly want to update the data, make sure you pass an appropriate `usage` hint in the arguments.
    void _update()
    {
        m_size = static_cast<GLsizei>(m_vertices.size());

        notf_check_gl(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
        if (m_size <= m_buffer_size) {
            // if the new data is smaller or of equal size than the last one, we can do a minimal update
            notf_check_gl(glBufferSubData(GL_ARRAY_BUFFER, /*offset = */ 0, m_size * sizeof(Vertex), &m_vertices[0]));
        }
        else {
            // otherwise we have to do a full update
            notf_check_gl(glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0],
                                       get_gl_usage(m_args.usage)));
        }
        m_buffer_size = std::max(m_buffer_size, m_size);

        notf_check_gl(glBindBuffer(GL_ARRAY_BUFFER, 0));

        m_vertices.clear();
        // do not shrink to size, if you call `update` once you are likely to call it again
    }

    /// Recursively define each attribute from the class' traits.
    template<size_t INDEX, typename FIRST, typename SECOND, typename... REST>
    void _init_array()
    {
        _define_attribute<INDEX, FIRST>();
        _init_array<INDEX + 1, SECOND, REST...>();
    }

    /// Recursion breaker.
    template<size_t INDEX, typename LAST>
    void _init_array()
    {
        _define_attribute<INDEX, LAST>();
    }

    /// Defines a single attribute.
    template<size_t INDEX, typename ATTRIBUTE>
    void _define_attribute()
    {
        // we cannot be sure that the tuple stores its fields in order :/
        // therefore we have to discover the offset for each element
        const auto offset = reinterpret_cast<std::intptr_t>(&std::get<INDEX>(m_vertices[0]))
                            - reinterpret_cast<std::intptr_t>(&m_vertices[0]);
        NOTF_ASSERT(offset >= 0);

        // not all attribute types fit into a single OpenGL ES attribute
        // larger types are stored in consecutive attribute locations
        const size_t attribute_count = sizeof(typename ATTRIBUTE::type) / sizeof(GLfloat);
        for (size_t multi = 0, last = (attribute_count / 4) + (attribute_count % 4 ? 1 : 0); multi < last; ++multi) {

            const GLint size = std::min(4, static_cast<int>(attribute_count) - static_cast<int>(multi * 4));
            NOTF_ASSERT(size >= 1 && size <= 4);

            // link the location in the array to the shader's attribute
            notf_check_gl(glEnableVertexAttribArray(ATTRIBUTE::location + multi));
            notf_check_gl(glVertexAttribPointer(
                ATTRIBUTE::location + multi, size, to_gl_type(typename ATTRIBUTE::type::element_t{}),
                ATTRIBUTE::normalized, static_cast<GLsizei>(sizeof(Vertex)),
                gl_buffer_offset(static_cast<size_t>(offset + (multi * 4 * sizeof(GLfloat))))));

            // define the attribute as an instance attribute
            if (m_args.per_instance) {
                notf_check_gl(glVertexAttribDivisor(ATTRIBUTE::location + multi, 1));
            }
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Vertices stored in the array.
    std::vector<Vertex> m_vertices;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_buffer_size = 0;
};

NOTF_CLOSE_NAMESPACE
