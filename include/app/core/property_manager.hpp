#pragma once

#include <tuple>

#include "app/core/property_graph.hpp"
#include "common/mpsc_queue.hpp"
#include "common/variant.hpp"

// property value types
#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/matrix3.hpp"
#include "common/matrix4.hpp"
#include "common/size2.hpp"
#include "common/tuple.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"

namespace notf {

//====================================================================================================================//

namespace property_manager_detail {

/// Tuple type containing all types that properties can contain.
/// Add new types here if you need to put another type into a property.
/// The type itself is never actually instantiated, it is only used to create the value- and expression-variants.
using notf_property_types = std::tuple<int, float, bool, std::string, Aabrf, Color, Matrix3f, Matrix4f, Size2f, Size2i,
                                       Vector2f, Vector3f, Vector4f>;

// -------------------------------------------------------------------------------------------------------------------//

template<typename TUPLE, std::size_t... N>
constexpr decltype(auto) make_value_variant_impl(const TUPLE&, std::index_sequence<N...>)
{
    return std::variant<typename std::tuple_element<N, TUPLE>::type...>{};
}

/// Turns a type list contained in a tuple into a variant of these types.
/// For example:
///     std::tuple<type1, type2, type3,...>
/// will be turned into
///     std::variant<type1, type2, type3,...>
template<typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
constexpr decltype(auto) make_value_variant(const std::tuple<Ts...>& tuple)
{
    return make_value_variant_impl(tuple, Indices{});
}

// -------------------------------------------------------------------------------------------------------------------//

template<typename TUPLE, std::size_t... N>
constexpr decltype(auto) make_function_variant_impl(const TUPLE&, std::index_sequence<N...>)
{
    return std::variant<std::function<typename std::tuple_element<N, TUPLE>::type(const PropertyGraph&)>...>{};
}

/// Turns a type list contained in a tuple into a variant of argument-less function types.
/// For example:
///     std::tuple<type1, type2, type3,...>
/// will be turned into
///     std::variant<std::function<type1()>, std::function<type2()>, std::function<type3()>,...>
template<typename... Ts, typename Indices = std::make_index_sequence<sizeof...(Ts)>>
constexpr decltype(auto) make_function_variant(const std::tuple<Ts...>& tuple)
{
    return make_function_variant_impl(tuple, Indices{});
}

} // namespace property_manager_detail

//====================================================================================================================//

class PropertyManager {

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// All types that a Property can have
    using ValueVariant
        = decltype(property_manager_detail::make_value_variant(property_manager_detail::notf_property_types{}));

    /// All expression types that a Property can have
    using ExpressionVariant
        = decltype(property_manager_detail::make_function_variant(property_manager_detail::notf_property_types{}));

    //----------------------------------------------------------------------------------------------------------------//

    /// Command object, used as data type in the MPSC queue.
    struct Command {

        /// First type, used for default constructed (or moved) command types.
        struct Empty {
            // no-op
        };

        // Create a new value (only requires the PropertyId).
        struct Create {
            // empty
        };

        // Set property value.
        struct SetValue {
            ValueVariant value;
        };

        /// Set expression command, requires the expression alongside its dependencies.
        struct SetExpression {
            ExpressionVariant expression;
            std::vector<PropertyId> dependencies;
        };

        /// Remove Command (only requires the PropertyId).
        struct Delete {
            // empty
        };

        /// Command type, is a variant of all the possible types of Commands.
        using Type = std::variant<Empty, Create, SetValue, SetExpression, Delete>;

        // methods ---------------------------------------------------------------------------------------------------//
        /// Default constructor (produces an invalid command).
        Command() : property(PropertyId::invalid()), type(Empty{}) {}

        /// Value Constructor.
        /// @param id       Property id.
        /// @param type     Command type.
        Command(PropertyId id, Type&& type) : property(id), type(std::move(type)) {}

        /// Move constructor.
        Command(Command&& other) : property(other.property), type(std::move(other.type)) { other.type = Empty{}; }

        // fields ----------------------------------------------------------------------------------------------------//
        /// Property id.
        const PropertyId property;

        /// Command type.
        Type type;
    };

    using CommandList = std::vector<Command>;

    //----------------------------------------------------------------------------------------------------------------//

    /// Events batch up their commands so that we can be certain that either all or none of them are in effect at any
    /// given time. Otherwise it would be possible to render a frame with some of an event's commands executed and
    /// others still in the queue.
    struct InternalBatch {

        /// Commands in this batch.
        CommandList commands;

        /// Creation time of the event issuing the batch.
        /// Is used for ordering.
        Time time;
    };

    //----------------------------------------------------------------------------------------------------------------//
public:
    /// Public batch object, used to create commands to modify the graph.
    class CommandBatch {
        friend class PropertyManager;

        // methods ---------------------------------------------------------------------------------------------------//
    private: // for Property Manager
        /// Constructor.
        /// @param graph    Graph to modify with the commands.
        /// @param time     Time of the event that creates the batch.
        CommandBatch(PropertyGraph& graph, Time time) : m_graph(graph), m_time(std::move(time)), m_commands() {}

        /// Strips the interal batch out of this public batch type in order to store it in the manager's queue.
        InternalBatch ingest() { return InternalBatch{std::move(m_commands), std::move(m_time)}; }

    public:
        /// Create a new property of a given type.
        /// Only allows the creation of properties of a type mentioned in the notf_property_types tuple type.
        /// @returns A typed id that contains the value type of the property for ease-of-use.
        template<typename value_t, typename = typename std::enable_if<is_one_of_tuple<
                                       value_t, property_manager_detail::notf_property_types>::value>::type>
        TypedPropertyId<value_t> create_property()
        {
            PropertyId id = m_graph.next_id();
            m_commands.emplace_back(id, Command::Create{});
            return TypedPropertyId<value_t>(id.value);
        }

        /// Set a new value for a given property.
        /// Uses fancy template-foo to allow the user to omit the property type, allow automatic conversion to the
        /// correct type and also to forbid wrong instantiations, where conversions would fail.
        /// @param id       Typed property id.
        /// @param value    New property value.
        template<typename value_t, typename T,
                 typename = typename std::enable_if<std::is_convertible<T, value_t>::value>::type>
        void set_property(const TypedPropertyId<value_t> id, T value)
        {
            m_commands.emplace_back(id, Command::SetValue{std::move(static_cast<value_t>(value))});
        }

        /// Sets a new expression for a given property.
        /// Until we have some sort of introspection (if C++ allows it at all, that is), you have to manually list all
        /// dependent property (ids) to ensure that the dirty propagation in the property graph works.
        /// Uses fancy template-foo to allow the user to omit the property type, allow automatic conversion to the
        /// correct type and also to forbid wrong instantiations, where conversions would fail.
        /// @param id           Typed property id.
        /// @param expression   New property expression
        /// @param dependencies All other properties that the expression relies on.
        template<typename value_t, typename T,
                 typename = typename std::enable_if<
                     std::is_convertible<T, std::function<value_t(const PropertyGraph&)>>::value>::type>
        void set_expression(const TypedPropertyId<value_t> id, T&& expression, std::vector<PropertyId>&& dependencies)
        {
            m_commands.emplace_back(id, Command::SetExpression{std::move(expression), std::move(dependencies)});
        }

        /// Deletes the property with the given id.
        /// @param id   Property to delete.
        void delete_property(PropertyId id) { m_commands.emplace_back(id, Command::Delete{}); }

        // fields
        // -----------------------------------------------------------------------------------------------------------//
    private:
        /// Graph to modify with the commands.
        PropertyGraph& m_graph;

        /// Creation time of the event issuing the batch.
        /// Is used for ordering.
        Time m_time;

        /// Commands in this batch.
        CommandList m_commands;
    };

    friend class CommandBatch;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default constructor.
    PropertyManager() = default;

    /// Create a new command batch to fill.
    CommandBatch create_batch(Time time) { return CommandBatch{m_graph, std::move(time)}; }

    /// Schedule the command batch for execution.
    void schedule_batch(CommandBatch&& batch) { m_batches.push(batch.ingest()); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The managed property graph.
    PropertyGraph m_graph;

    /// Multiple producer / single consumer queue used for interthread communication.
    MpscQueue<InternalBatch> m_batches;
};

} // namespace notf
