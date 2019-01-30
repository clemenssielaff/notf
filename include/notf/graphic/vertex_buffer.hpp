#pragma once

#include "notf/meta/concept.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/arithmetic.hpp"

#include "notf/graphic/fwd.hpp"

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
                    // success
                    return declval<typename Policy::type>();
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
            if constexpr ((std::is_convertible_v<decltype(Policy::is_normalized), bool>)) {
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

// any vertex buffer ================================================================================================ //

/// Base class for all VertexBuffers.
/// See `VertexBuffer` for implementation details.
class AnyVertexBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Whether attributes in this buffer are applied to each vertex or instance.
    enum class Application {
        PER_VERTEX,
        PER_INSTANCE,
    };

    /// Arguments for the vertex array.
    struct Args {
        /// The expected usage of the data.
        GLUsage usage = GLUsage::STATIC_DRAW;

        /// Whether attributes in this buffer are applied to each vertex or instance.
        Application application = Application::PER_VERTEX;
    };

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param name         Name of this VertexBuffer.
    /// @param args         Arguments used to initialize this VertexBuffer.
    /// @throws OpenGLError If the VBO could not be allocated.
    AnyVertexBuffer(std::string name, Args args);

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyVertexBuffer);

    /// Destructor.
    virtual ~AnyVertexBuffer();

    /// Name of this VertexBuffer.
    const std::string& get_name() const { return m_name; }

    /// OpenGL handle of the vertex buffer.
    VertexBufferId get_id() const { return m_id; }

    /// Whether attributes in this buffer are applied to each vertex or instance.
    Application get_application() const { return m_args.application; }

protected:
    /// Registers a new VertexBuffer with the ResourceManager.
    static void _register_ressource(AnyVertexBufferPtr vertex_buffer);

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// Arguments used to initialize this VertexBuffer.
    const Args m_args;

    /// Name of this VertexBuffer.
    std::string m_name;

    /// OpenGL ID of the managed vertex buffer object.
    VertexBufferId m_id = VertexBufferId::invalid();
};

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
///     using MyVertexBuffer = VertexBuffer<PositionAttribute, AlphaAttribute>;
///
/// VertexBuffer are shared among all GraphicContexts and managed through `shared_ptr`s. When TheGraphicSystem goes
/// out of scope, all VertexBuffer will be deallocated. Trying to modify a deallocated VertexBuffer will throw an
/// exception.
template<class... AttributePolicies>
class VertexBuffer : public AnyVertexBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// All AttributePolicies as passed by the user.
    using user_policies = std::tuple<AttributePolicies...>;

    /// All validated Attribute policies.
    using policies = decltype(detail::vertex_buffer_policy_factory(user_policies()));

    /// A tuple containing one entry for each Attribute type in order.
    using vertex_t = decltype(detail::extract_attribute_policy_types(policies{}));

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(VertexBuffer);

    /// Constructor.
    /// @param name         Name of this VertexBuffer.
    /// @param args         Arguments used to initialize this VertexBuffer.
    /// @throws OpenGLError If the VBO could not be allocated.
    VertexBuffer(std::string name, Args args) : AnyVertexBuffer(std::move(name), std::move(args)) {}

public:
    /// Factory.
    /// @param name             Name of this VertexBuffer.
    /// @param args             Arguments used to initialize this VertexBuffer.
    /// @throws OpenGLError     If the VBO could not be allocated.
    /// @throws ResourceError   If another VertexBuffer with the same name already exist.
    static VertexBufferPtr<AttributePolicies...> create(std::string name, Args args) {
        auto result = _create_shared(std::move(name), std::move(args));
        _register_ressource(result);
        return result;
    }

    /// Write-access to the vertex buffer.
    std::vector<vertex_t>& write() {
        m_local_hash = 0;
        // TODO: VertexBuffer::write should return a dedicated "write" object, that keeps track of changes.
        // This way, it can determine whether the local hash needs to be re-calculated in `apply` or not (by setting
        // `m_local_hash` to zero on each change) and we might even be able to use multiple smaller calls to
        // `glBufferSubData` instead of a single big one, just from data we collect automatically from the writer.
        return m_buffer;
    }

    /// Updates the server data with the client's.
    /// If no change occured or the client's data is empty, this method is a noop.
    void apply() {
        // noop if there is nothing to update
        if (m_buffer.empty()) { return; }

        // update the local hash on request
        if (0 == m_local_hash) {
            m_local_hash = hash(m_buffer);

            // noop if the data on the server is still current
            if (m_local_hash == m_server_hash) { return; }
        }

        // bind and eventually unbind the vertex buffer
        struct VertexBufferGuard {
            VertexBufferGuard(const GLuint id) { NOTF_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, id)); }
            ~VertexBufferGuard() { NOTF_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, 0)); }
        };
        NOTF_GUARD(VertexBufferGuard(get_id().get_value()));

        // upload the buffer data
        const GLsizei buffer_size = m_buffer.size() * sizeof(vertex_t);
        if (buffer_size <= m_server_size) {
            NOTF_CHECK_GL(glBufferSubData(GL_ARRAY_BUFFER, /*offset = */ 0, buffer_size, &m_buffer.front()));
        } else {
            NOTF_CHECK_GL(glBufferData(GL_ARRAY_BUFFER, buffer_size, &m_buffer.front(), get_gl_usage(m_args.usage)));
            m_server_size = buffer_size;
        }
        // TODO: It might be better to use two buffers for each VertexBuffer object.
        // One that is currently rendered from, one that is written into. Note on the OpenGL reference:
        // (https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glBufferSubData.xhtml)
        //
        //      Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store
        //      updates. If any rendering in the pipeline makes reference to data in the buffer object being updated by
        //      glBufferSubData, especially from the specific region being updated, that rendering must drain from the
        //      pipeline before the data store can be updated.
        //

        m_server_hash = m_local_hash;
    }

private:
    /// Define all Attributes.
    /// This method should only be called from a VertexObject because it requires a bound VAO.
    /// @throws OpenGLError If no VAO is currently bound.
    void _define_attributes() const {
        { // make sure there is a bound VertexObject
            GLint current_vao = 0;
            NOTF_CHECK_GL(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao));
            if (!current_vao) {
                NOTF_THROW(OpenGLError, "Cannot initialize a VertexBuffer without a bound VertexObject");
            }
        }

        // bind, but do not unbind the vertex buffer, we expect the VAO to take care of that after it has unbound itself
        NOTF_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, m_id.get_value()));

        // define all attributes
        _define_attribute<0>();
    }

    // TODO: attribute definition should happen INSIDE a VertexObject

    /// Define a single Attribute in the buffer.
    template<size_t Index>
    void _define_attribute() {
        if constexpr (Index < std::tuple_size_v<policies>) {
            using AttributePolicy = std::tuple_element<Index, policies>;
            using ValueType = typename AttributePolicy::type;
            using ElementType = typename AttributePolicy::element_t;
            constexpr GLuint attribute_location = AttributePolicy::location;
            constexpr bool attribute_is_normalized = AttributePolicy::is_normalized;

            // we cannot be certain that the std library implementation places tuple fields in memory in order,
            // therefore we have to discover the memory offset for ourselves
            static constexpr vertex_t test_vertex;
            const std::uintptr_t memory_offset = to_number(&std::get<Index>(test_vertex)) - to_number(&test_vertex);
            NOTF_ASSERT(memory_offset >= 0);

            // not all value types fit into a single OpenGL ES attribute which is 4 floats wide,
            // larger types are stored in consecutive attribute locations
            static_assert(sizeof(ValueType) % sizeof(GLfloat) == 0);
            const uint attribute_width = sizeof(ValueType) / sizeof(GLfloat);
            for (uint attribute_offset = 0, last = (attribute_width + 3) / 4; attribute_offset < last;
                 ++attribute_offset) {
                const auto buffer_offset = gl_buffer_offset(memory_offset + (attribute_offset * 4 * sizeof(GLfloat)));

                // link a location in the buffer to an attribute slot
                NOTF_CHECK_GL(glEnableVertexAttribArray(attribute_location + attribute_offset));
                NOTF_CHECK_GL(glVertexAttribPointer(                  //
                    attribute_location + attribute_offset,            // location
                    min(4, attribute_width - (attribute_offset * 4)), // size
                    to_gl_type(ElementType()),                        // type
                    attribute_is_normalized,                          // normalized
                    static_cast<GLsizei>(sizeof(vertex_t)),           // stride
                    buffer_offset                                     // offset
                    ));

                // if this buffer is applied per instance, let OpenGL know
                if (m_args.application == Application::PER_INSTANCE) {
                    NOTF_CHECK_GL(glVertexAttribDivisor(attribute_location + attribute_offset, 1));
                }
            }

            // define remaining attributes
            _define_attribute<Index + 1>();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertices stored in the buffer.
    std::vector<vertex_t> m_buffer;

    /// Size in bytes of the buffer allocated on the server.
    GLsizei m_server_size = 0;

    /// Hash of the current data held by the application.
    size_t m_local_hash = 0;

    /// Hash of the data that was last uploaded to the GPU.
    size_t m_server_hash = 0;
};

NOTF_CLOSE_NAMESPACE

// common_type ====================================================================================================== //

/// std::common_type specializations for AnyVertexBufferPtr subclasses.
template<class... Lhs, class... Rhs>
struct std::common_type<::notf::VertexBuffer<Lhs...>, ::notf::VertexBufferPtr<Rhs...>> {
    using type = ::notf::AnyVertexBufferPtr;
};
