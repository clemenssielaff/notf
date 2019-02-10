#pragma once

#include "notf/meta/concept.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/tuple.hpp"

#include "notf/common/arithmetic.hpp"

#include "notf/graphic/opengl_buffer.hpp"

NOTF_OPEN_NAMESPACE

// vertex buffer policy factory ===================================================================================== //

namespace detail {

template<class Policy>
class AttributePolicyFactory {

    NOTF_CREATE_TYPE_DETECTOR(type);
    static constexpr auto create_type() {
        if constexpr (!has_type_v<Policy>) {
            static_assert(always_false_v<Policy>,
                          "An AttributePolicy must define the Attribute value type `using type = <type>;`");
        } else {
            using ValueType = typename Policy::type;
            if constexpr (!std::is_trivial_v<ValueType>) {
                static_assert(always_false_v<Policy>, "The Attribute value type must be trivial");
            } else {
                if constexpr (sizeof(ValueType) % sizeof(GLfloat) != 0) {
                    static_assert(always_false_v<Policy>,
                                  "The size of an Attribute value type must be divisible by `sizeof(GLfloat)`");
                } else {
                    return declval<ValueType>(); // success
                }
            }
        }
    }

    NOTF_CREATE_FIELD_DETECTOR(location);
    static constexpr auto create_location() {
        if constexpr (std::conjunction_v<has_location<Policy>, //
                                         std::is_convertible<decltype(Policy::location), GLuint>>) {
            return static_cast<GLuint>(Policy::location);
        } else {
            static_assert(always_false_v<Policy>,
                          "An AttributePolicy must contain a the location of the attribute as field "
                          "`constexpr static GLuint location = <location>;`");
        }
    }

    NOTF_CREATE_FIELD_DETECTOR(is_normalized);
    static constexpr auto create_is_normalized() {
        if constexpr (has_is_normalized_v<Policy>) {
            if constexpr (std::is_convertible_v<decltype(Policy::is_normalized), bool>) {
                return static_cast<bool>(Policy::is_normalized);
            } else {
                static_assert(
                    always_false_v<Policy>,
                    "The normalization flag of an AttributePolicy must be a boolean:"
                    "`constexpr static bool is_normalized = <is_normalized>;`. If such a field is not provided, the flag"
                    " is defaulted to false.");
            }
        } else {
            return false; // by default
        }
    }

    static constexpr auto create_element_t() {
        if constexpr (has_type_v<Policy>) {
            using ValueType = typename Policy::type;
            if constexpr (is_arithmetic_type<ValueType>) {
                return declval<typename ValueType::element_t>();
            } else if constexpr (std::is_arithmetic_v<ValueType>) {
                return declval<ValueType>();
            } else {
                static_assert(always_false_v<Policy>,
                              "Failed to identify the element type of the given Attribute value type");
            }
        }
    }

public:
    /// Policy type
    struct AttributePolicy {

        /// Type used to store the trait value.
        using type = std::decay_t<decltype(create_type())>;

        /// Element type of the Attribute's value type.
        /// For a `V2f` this would be `float`, for a `M3i` it would be `int` and `float` remains `float`.
        /// This type does not have to be provided by the user as it is automatically detected by the factory.
        using element_t = std::decay_t<decltype(create_element_t())>;

        /// Location of the attribute in the shader.
        constexpr static GLuint location = create_location();

        /// Vertex attributes are internally stored as a single-precision floating-point number before being used in a
        /// vertex shader. If the data type indicates that the vertex attribute is not a float, then the vertex
        /// attribute will be converted to a single-precision floating-point number before it is used in a vertex
        /// shader. The `is_normalized` flag controls the conversion of the non-float vertex attribute data to a single
        /// precision floating-point value. If the flag is false, the vertex data are converted directly to a
        /// floating-point value. This would be similar to casting the variable that is not a float type to float. If
        /// the flag is true, the integer format is mapped to the range [-1,1] (for signed values) or [0,1] (for
        /// unsigned values).
        /// See https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glVertexAttribPointer.xhtml
        constexpr static bool is_normalized = create_is_normalized();
    };
};

/// Extracts the value types from a variadic list of traits.
template<class... Ts>
constexpr auto vertex_buffer_policy_factory(const std::tuple<Ts...>&) {
    static_assert(sizeof...(Ts) > 0, "A VertexBuffer type must contain at least one AttributePolicy");
    return std::tuple<typename AttributePolicyFactory<Ts>::AttributePolicy...>();
}

template<class Tuple, size_t... Index>
constexpr auto _extract_attribute_policy_types_impl(const Tuple&, std::index_sequence<Index...>) {
    return std::tuple<typename std::tuple_element_t<Index, Tuple>::type...>();
}

/// Extract all types from the Attribute policy tuple.
template<class... Ts, class Indices = std::make_index_sequence<sizeof...(Ts)>>
constexpr auto extract_attribute_policy_types(const std::tuple<Ts...>& tuple) {
    return _extract_attribute_policy_types_impl(tuple, Indices{});
}

} // namespace detail

// vertex buffer ==================================================================================================== //

/// Example usage:
///
///     struct PositionAttribute {
///         using type = V2f;
///         constexpr static GLuint location = 0;
///         constexpr static bool is_normalized = false;
///     };
///     struct AlphaAttribute {
///         using type = int;
///         constexpr static GLuint location = 1;
///         constexpr static bool is_normalized = true;
///     };
///     auto MyVertexBuffer = create_vertex_buffer<PositionAttribute, AlphaAttribute>("my_buffer");
///
/// VertexBuffer are shared among all GraphicContexts and managed through `shared_ptr`s. When TheGraphicSystem goes
/// out of scope, all VertexBuffer will be deallocated. Trying to modify a deallocated VertexBuffer will throw an
/// exception.
template<class AttributePolicies, class Vertex>
class VertexBuffer : public OpenGLBuffer<detail::OpenGLBufferType::VERTEX, Vertex> {

    friend Accessor<VertexBuffer, VertexObject>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(VertexBuffer);

    /// All validated Attribute policies.
    using policies = AttributePolicies;

    /// A tuple containing one entry for each Attribute policy in order.
    using vertex_t = Vertex;

    /// The expected usage of the data stored in this buffer.
    using UsageHint = typename detail::AnyOpenGLBuffer::UsageHint;

private:
    /// Base OpenGLBuffer class.
    using super_t = OpenGLBuffer<detail::OpenGLBufferType::VERTEX, Vertex>;

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexBuffer);

    /// Constructor.
    /// @param name             Human-readable name of this OpenGLBuffer.
    /// @param usage_hint       The expected usage of the data stored in this buffer.
    /// @param is_per_instance  Whether the data held in this buffer is applied per vertex (false -> the default)
    ///                         or per instance (true).
    /// @throws OpenGLError     If the buffer could not be allocated.
    VertexBuffer(std::string name, const UsageHint usage_hint, const bool is_per_instance)
        : super_t(std::move(name), usage_hint), m_is_per_instance(is_per_instance) {}

public:
    /// Factory.
    /// @param name             Human-readable name of this OpenGLBuffer.
    /// @param usage_hint       The expected usage of the data stored in this buffer.
    /// @param is_per_instance  Whether the data held in this buffer is applied per vertex (false -> the default)
    ///                         or per instance (true).
    /// @throws OpenGLError     If the buffer could not be allocated.
    static auto create(std::string name, const UsageHint usage_hint = UsageHint::DEFAULT,
                       const bool is_per_instance = false) {
        return _create_shared(std::move(name), usage_hint, is_per_instance);
    }

private:
    /// Binds the VertexBuffer to the bound VertexObject.
    /// @throws OpenGLError If no VAO is bound.
    void _bind_to_vao() {
        { // make sure there is a bound VertexObject
            GLint current_vao = 0;
            NOTF_CHECK_GL(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(OpenGLError, "Cannot initialize a VertexBuffer without an active VertexObject");
            }
        }
        NOTF_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, this->_get_handle()));
        _define_attributes();
    }

    /// Define a single Attribute in the buffer.
    template<size_t Index = 0>
    void _define_attributes() const {
        if constexpr (Index < std::tuple_size_v<vertex_t>) {
            using AttributePolicy = std::tuple_element_t<Index, policies>;
            using ValueType = typename AttributePolicy::type;
            using ElementType = typename AttributePolicy::element_t;
            constexpr GLuint attr_location = AttributePolicy::location;
            constexpr bool attr_is_normalized = AttributePolicy::is_normalized;

            // we cannot be certain that the std library implementation places tuple fields into memory in order,
            // therefore we have to discover the memory offset for ourselves
            static constexpr vertex_t test_vertex;
            const std::uintptr_t memory_offset = to_number(&std::get<Index>(test_vertex)) - to_number(&test_vertex);
            NOTF_ASSERT(memory_offset >= 0);

            // not all value types fit into a single OpenGL ES attribute which is 4 floats wide,
            // larger types are stored in consecutive attribute locations
            static_assert(sizeof(ValueType) % sizeof(GLfloat) == 0);
            const uint attr_width = sizeof(ValueType) / sizeof(GLfloat);
            for (uint attr_offset = 0, last = (attr_width + 3) / 4; attr_offset < last; ++attr_offset) {
                const auto buffer_offset = gl_buffer_offset(memory_offset + (attr_offset * 4 * sizeof(GLfloat)));

                // link a location in the buffer to an attribute slot
                NOTF_CHECK_GL(glEnableVertexAttribArray(attr_location + attr_offset));
                NOTF_CHECK_GL(glVertexAttribPointer(        //
                    attr_location + attr_offset,            // location
                    min(4, attr_width - (attr_offset * 4)), // size
                    to_gl_type(ElementType()),              // type
                    attr_is_normalized,                     // normalized
                    static_cast<GLsizei>(sizeof(vertex_t)), // stride
                    buffer_offset                           // offset
                    ));

                // if this buffer is applied per instance, let OpenGL know
                if (m_is_per_instance) { NOTF_CHECK_GL(glVertexAttribDivisor(attr_location + attr_offset, 1)); }
            }

            // define remaining attributes
            _define_attributes<Index + 1>();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Whether the data held in this buffer is applied per vertex (false -> the default) or per instance (true).
    const bool m_is_per_instance;
};

/// VertexBuffer factory.
/// @param name                    Human-readable name of this OpenGLBuffer.
/// @param usage_hint              The expected usage of the data stored in this buffer.
/// @param is_per_instance   Whether the data held in this buffer is applied per vertex (false -> the default)
///                                or per instance (true).
/// @throws OpenGLError            If the buffer could not be allocated.
template<class... AttributePolicies>
auto make_vertex_buffer(std::string name,
                        const AnyVertexBuffer::UsageHint usage_hint = AnyVertexBuffer::UsageHint::DEFAULT,
                        const bool is_per_instance = false) {
    using attribute_policies = decltype(detail::vertex_buffer_policy_factory(std::tuple<AttributePolicies...>{}));
    using vertex_t = decltype(detail::extract_attribute_policy_types(attribute_policies{}));
    return VertexBuffer<attribute_policies, vertex_t>::create(std::move(name), usage_hint, is_per_instance);
}

/// VertexBuffer type produced by `make_vertex_buffer` with the given template arguments.
template<class... AttributePolicies>
using vertex_buffer_t = typename decltype(make_vertex_buffer<AttributePolicies...>(""))::element_type;

// accessors ======================================================================================================== //

template<class AttributePolicies, class Vertex>
class Accessor<VertexBuffer<AttributePolicies, Vertex>, VertexObject> {
    friend VertexObject;

    /// Binds the VertexBuffer to the bound VertexObject.
    /// @throws OpenGLError If no VAO is bound.
    static void bind_to_vao(VertexBuffer<AttributePolicies, Vertex>& buffer) { buffer._bind_to_vao(); }
};

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyVertexBufferPtr subclasses.
template<class LeftAttrs, class LeftVertex, class RightAttrs, class RightVertex>
struct std::common_type<::notf::VertexBufferPtr<LeftAttrs, LeftVertex>,
                        ::notf::VertexBufferPtr<RightAttrs, RightVertex>> {
    using type = ::notf::AnyVertexBufferPtr;
};
