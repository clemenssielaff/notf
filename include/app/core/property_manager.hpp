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

    // types -------------------------------------------------------------------------------------------------------//
private:
    /// All types that a Property can have
    using ValueVariant
        = decltype(property_manager_detail::make_value_variant(property_manager_detail::notf_property_types{}));

    /// All expression types that a Property can have
    using ExpressionVariant
        = decltype(property_manager_detail::make_function_variant(property_manager_detail::notf_property_types{}));

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
            std::function<void()> expression;
            std::vector<PropertyId> dependencies;
        };

        /// Remove Command (only requires the PropertyId).
        struct Remove {
            // empty
        };

        /// Default constructor (produces an invalid command).
        Command() : time(Time::invalid()), property(PropertyId::invalid()), type(Empty{}) {}

        /// Move constructor.
        Command(Command&& other) : time(other.time), property(other.property), type(std::move(other.type))
        {
            other.type = Empty{};
        }

        /// Command type, is a variant of all the possible types of Commands.
        using Type = std::variant<Empty, Create, SetValue, SetExpression, Remove>;

        /// Creation time of the event issuing the Command.
        /// Is used to order the Commands.
        const Time time;

        /// Property id.
        const PropertyId property;

        /// Command type.
        Type type;
    };

    using CommandList = std::vector<Command>;

public:
    /// Events batch up their commands so that we can be certain that either all or none of them are in effect at any
    /// given time. Otherwise it would be possible to render a frame with some of an event's commands executed and
    /// others still in the queue.
    class CommandBatch {
        friend class PropertyManager;

        // methods ---------------------------------------------------------------------------------------------------//
    private: // for Property Manager
        /// Constructor.
        CommandBatch(PropertyGraph& graph) : m_graph(graph), m_commands() {}

    public:
        template<typename value_t>
        PropertyId create_property()
        {
            PropertyId id = m_graph.next_id();

            m_graph.add_property<value_t>(id);
        }

        // fields ----------------------------------------------------------------------------------------------------//
    private:
        /// Graph to modify with the commands.
        PropertyGraph& m_graph;

        /// Commands in this batch.
        CommandList m_commands;

//        const Time m_time;

        // TODO: continue here
        // time should be per batch, not per Command. We might need an external and an internal batch type for that
        // (the external also contains the graph to operate on).
        // From there on we just need to CRUD operations as methods on the external batch and take it for a psin!
        // Also, you might want to check if it possible to add another type to the ID type so that not all method
        // signatures of the external batch need to be qualified with explicit templates.
    };

    friend class CommandBatch;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default constructor.
    PropertyManager() = default;

    /// Create a new command batch to fill.
    CommandBatch create_batch() { return CommandBatch{m_graph}; }

    /// Schedule the command batch for execution.
    void schedule_batch(CommandBatch&& batch) { m_batches.push(std::move(batch.m_commands)); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The managed property graph.
    PropertyGraph m_graph;

    /// Multiple producer / single consumer queue used for interthread communication.
    MpscQueue<CommandList> m_batches;
};

} // namespace notf
