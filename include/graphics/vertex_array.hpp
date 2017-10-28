#pragma once

#include <assert.h>

#include "common/exception.hpp"
#include "common/meta.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"

namespace notf {

//====================================================================================================================//

namespace detail {

class PrefabFactoryImpl;

template<typename TUPLE, std::size_t... I>
decltype(auto) extract_trait_types_impl(const TUPLE&, std::index_sequence<I...>) {
	return std::tuple<typename std::tuple_element<I, TUPLE>::type::type...>{};
}

/** Extracts the value types from a variadic list of traits. */
template<typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
decltype(auto) extract_trait_types(const std::tuple<Ts...>& tuple) {
	return extract_trait_types_impl(tuple, Indices{});
}

} // namespace detail

//====================================================================================================================//

/// @brief Definitions used to identify VertexArray traits to the Geometry factory.
/// Used to tell the GeometryFactory how to construct a VertexArray<Traits...>::Vertex instance.
struct AttributeKind {

	/// @brief Vertex position in model space.
	struct Position {};

	/// @brief Vertex normal vector.
	struct Normal {};

	/// @brief Vertex color.
	struct Color {};

	/// @brief Texture coordinate.
	struct TexCoord {};

	/// @brief Catch-all for other attribute kinds.
	/// Does not impose any restrictions on the Trait::type.
	struct Other {};
};

//====================================================================================================================//

/// @brief Base of all attribute kinds.
struct AttributeTrait {

	/// @brief Location of the attribute in the shader.
	constexpr static uint location = std::numeric_limits<GLuint>::max();

	/// @brief Whether the value type is normalized or not.
	/// See https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribPointer.xhtml
	constexpr static bool normalized = false;

	/// @brief Type used to store the trait value.
	using type = void;

	/// @brief Attribute kind, is used by the GeometryFactory to identify the trait.
	using kind = AttributeKind::Other;
};

//====================================================================================================================//

/// @brief VertexArray baseclass, so other objects can hold pointers to any type of VertexArray.
class VertexArrayType {

	// types ---------------------------------------------------------------------------------------------------------//
public:
	/// @brief Arguments for the vertex array.
	struct Args {

		/// @brief The expected usage of the data.
		/// Must be one of:
		/// GL_STREAM_DRAW    GL_STATIC_DRAW    GL_DYNAMIC_DRAW
		/// GL_STREAM_READ    GL_STATIC_READ    GL_DYNAMIC_READ
		/// GL_STREAM_COPY    GL_STATIC_COPY    GL_DYNAMIC_COPY
		GLenum usage = GL_STATIC_DRAW;

		/// @brief Whether this array is per-vertex or per-instance.
		bool per_instance = false;
	};

	// methods -------------------------------------------------------------------------------------------------------//
public:
	DISALLOW_COPY_AND_ASSIGN(VertexArrayType)

	/// @brief Destructor.
	virtual ~VertexArrayType();

	/// @brief Initializes the VertexArray.
	/// @throws std::runtime_error   If the VBO could not be allocated.
	/// @throws std::runtime_error   If no VAO object is currently bound.
	virtual void init() = 0;

	/// @brief OpenGL handle of the vertex buffer.
	GLuint id() const { return m_vbo_id; }

	/// @brief Number of elements in the array.
	GLsizei size() const { return m_size; }

protected:
	/// @brief Constructor.
	VertexArrayType(Args&& args) : m_args(std::move(args)), m_vbo_id(0), m_size(0) {}

	// fields --------------------------------------------------------------------------------------------------------//
protected:
	/// @brief Arguments used to initialize the vertex array.
	const Args m_args;

	/// @brief OpenGL handle of the vertex buffer. */
	GLuint m_vbo_id;

	/// @brief Number of elements in the array. */
	GLsizei m_size;

	/// @brief Invalid attribute ID.
	static constexpr GLuint INVALID_ID = std::numeric_limits<GLuint>::max();
};

//====================================================================================================================//

/// @brief The Vertex array manages an array of vertex attributes.
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

	// types ---------------------------------------------------------------------------------------------------------//
public:
	/// @brief Traits defining the array layout.
	using Traits = std::tuple<Ts...>;

	/// @brief A tuple containing one array for each trait.
	using Vertex = decltype(detail::extract_trait_types(Traits{}));

	// methods -------------------------------------------------------------------------------------------------------//
public:
	/// @brief Constructor.
	VertexArray(Args&& args = {}) : VertexArrayType(std::forward<Args>(args)), m_vertices(), m_buffer_size(0) {}

	/// @brief Initializes the VertexArray.
	/// @throws std::runtime_error   If the VBO could not be allocated.
	/// @throws std::runtime_error   If no VAO object is currently bound.
	virtual void init() override {
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
		_init_array<0, Ts...>();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		check_gl_error();

		m_vertices.clear();
		m_vertices.shrink_to_fit();
	}

	/// @brief Updates the data in the vertex array.
	/// If you regularly want to update the data, make sure you pass an appropriate `usage` hint in the arguments.
	/// @param data  New data to upload.
	/// @throws std::runtime_error   If the VertexArray is not yet initialized.
	/// @throws std::runtime_error   If no VAO object is currently bound.
	void update(std::vector<Vertex>&& data) {
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

private:
	/// @brief Recursively define each attribute from the class' traits.
	template<size_t INDEX, typename FIRST, typename SECOND, typename... REST>
	void _init_array() {
		_define_attribute<INDEX, FIRST>();
		_init_array<INDEX + 1, SECOND, REST...>();
	}

	/// @brief Recursion breaker.
	template<size_t INDEX, typename LAST>
	void _init_array() {
		_define_attribute<INDEX, LAST>();
	}

	/// @brief Defines a single attribute.
	template<size_t INDEX, typename ATTRIBUTE>
	void _define_attribute() {
		// we cannot be sure that the tuple stores its fields in order :/
		// therefore we have to discover the offset for each element
		const auto offset = reinterpret_cast<std::intptr_t>(&std::get<INDEX>(m_vertices[0]))
		                    - reinterpret_cast<std::intptr_t>(&m_vertices[0]);
		assert(offset >= 0);

		// not all attribute types fit into a single OpenGL ES attribute
		// larger types are stored in consecutive attribute locations
		const size_t attribute_count = sizeof(typename ATTRIBUTE::type) / sizeof(GLfloat);
		for (size_t multi = 0; multi < (attribute_count / 4) + (attribute_count % 4 ? 1 : 0); ++multi) {

			const GLint size = std::min(4, static_cast<int>(attribute_count) - static_cast<int>((multi * 4)));
			assert(size >= 1 && size <= 4);

			// link the location in the array to the shader's attribute
			glEnableVertexAttribArray(ATTRIBUTE::location + multi);
			glVertexAttribPointer(ATTRIBUTE::location + multi, size, to_gl_type(typename ATTRIBUTE::type::element_t{}),
			                      ATTRIBUTE::normalized, static_cast<GLsizei>(sizeof(Vertex)),
			                      gl_buffer_offset(static_cast<size_t>(offset + (multi * 4 * sizeof(GLfloat)))));

			// define the attribute as an instance attribute
			if (m_args.per_instance) {
				glVertexAttribDivisor(ATTRIBUTE::location + multi, 1);
			}
		}
	}

	// fields --------------------------------------------------------------------------------------------------------//
private:
	/// @brief Vertices stored in the array.
	std::vector<Vertex> m_vertices;

	/// @brief Size in bytes of the buffer allocated on the server.
	GLsizei m_buffer_size;
};

} // namespace notf
