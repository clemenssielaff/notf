#pragma once

#include "notf/app/graph/any_node.hpp"
#include "notf/app/graph/property.hpp"

NOTF_OPEN_NAMESPACE

// node policy factory ============================================================================================== //

namespace detail {

template<template<class> class, class...>
struct instantiate_shared;
template<template<class> class Template, class... Ts>
struct instantiate_shared<Template, std::tuple<Ts...>> {
    using type = std::tuple<std::shared_ptr<Template<Ts>>...>;
};

template<template<class> class, class...>
struct instantiate_unique;
template<template<class> class Template, class... Ts>
struct instantiate_unique<Template, std::tuple<Ts...>> {
    using type = std::tuple<std::unique_ptr<Template<Ts>>...>;
};

template<class Policy>
class NodePolicyFactory { // TODO: (NodePolicyFactory) check that all names are unique

    NOTF_CREATE_TYPE_DETECTOR(properties);
    static constexpr auto create_properties() {
        if constexpr (has_properties_v<Policy>) {
            return declval<typename instantiate_unique<Property, typename Policy::properties>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(slots);
    static constexpr auto create_slots() {
        if constexpr (has_slots_v<Policy>) {
            return declval<typename instantiate_unique<Slot, typename Policy::slots>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(signals);
    static constexpr auto create_signals() {
        if constexpr (has_signals_v<Policy>) {
            return declval<typename instantiate_shared<Signal, typename Policy::signals>::type>();
        } else {
            return std::tuple<>();
        }
    }

public:
    /// Policy type
    struct NodePolicy {

        /// All Properties of the Node type.
        using properties = decltype(create_properties());

        /// All Slots of the Node type.
        using slots = decltype(create_slots());

        /// All Signals of the Node type.
        using signals = decltype(create_signals());
    };
};

struct EmptyNodePolicy {};

} // namespace detail

// node ============================================================================================================= //

template<class Policy = detail::EmptyNodePolicy>
class Node : public AnyNode {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy used to create this Node type.
    using node_policy_t = typename detail::NodePolicyFactory<Policy>::NodePolicy;

protected:
    /// Tuple of Property types managed by this Node.
    using NodeProperties = typename node_policy_t::properties;

    /// Type of a specific Property of this Node.
    template<size_t I>
    using node_property_t = typename std::tuple_element_t<I, NodeProperties>::element_type;

    /// Tuple of Slot types managed by this Node.
    using NodeSlots = typename node_policy_t::slots;

    /// Type of a specific Slot of this Node.
    template<size_t I>
    using node_slot_t = typename std::tuple_element_t<I, NodeSlots>::element_type;

    /// Tuple of Signals types managed by this Node.
    using NodeSignals = typename node_policy_t::signals;

    /// Type of a specific Signal of this Node.
    template<size_t I>
    using node_signal_t = typename std::tuple_element_t<I, NodeSignals>::element_type;

    // helper --------------------------------------------------------------------------------- //
private:
    /// Extracts the type from a Tuple, or the element_type if the type is a shared/unique pointer.
    template<class Tuple, size_t I>
    struct _TypeIdentifier {
        static constexpr const auto& get() {
            using raw_type = typename std::tuple_element_t<I, Tuple>;
            if constexpr (is_shared_ptr_v<raw_type> || is_unique_ptr_v<raw_type>) {
                return declval<typename raw_type::element_type>();
            } else {
                return declval<raw_type>();
            }
        }
        using type = std::decay_t<decltype(get())>;
    };
    template<class Tuple, size_t I>
    using identify_type_t = typename _TypeIdentifier<Tuple, I>::type;

protected:
    /// Finds the index of the tuple element by its static ConstString field `name`.
    /// @param name Name to look for.
    /// @returns    Index of the matching element or the size of the tuple if not found.
    template<class Tuple, size_t I = 0, char... Cs>
    static constexpr size_t _get_index(StringType<Cs...> name) noexcept {
        if constexpr (I < std::tuple_size_v<Tuple>) {
            using type = identify_type_t<Tuple, I>;
            if (type::name == name) {
                return I;
            } else {
                return _get_index<Tuple, I + 1>(name);
            }
        } else {
            return std::tuple_size_v<Tuple>; // not found
        }
    }

    /// Finds the index of the tuple element by the hash value of its static ConstString field `name`.
    /// @param hash Hash value to look for.
    /// @returns    Index of the matching element or the size of the tuple if not found.
    template<class Tuple, size_t I = 0>
    static constexpr size_t _get_index(const size_t hash) noexcept {
        if constexpr (I < std::tuple_size_v<Tuple>) {
            using type = identify_type_t<Tuple, I>;
            if (type::name.get_hash() == hash) {
                return I;
            } else {
                return _get_index<Tuple, I + 1>(hash);
            }
        } else {
            return std::tuple_size_v<Tuple>; // not found
        }
    }

    /// Finds the index of the tuple element by comparing the value of its static ConstString field `name` to a runtime
    /// string.
    /// @param name Runtime string to look for.
    /// @returns    Index of the matching element or the size of the tuple if not found.
    template<class Tuple, size_t I = 0>
    static size_t _get_index(const std::string& name) noexcept {
        if constexpr (I < std::tuple_size_v<Tuple>) {
            using type = identify_type_t<Tuple, I>;
            if (name.compare(type::name.c_str())) {
                return I;
            } else {
                return _get_index<Tuple, I + 1>(name);
            }
        } else {
            return std::tuple_size_v<Tuple>; // not found
        }
    }

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Node(valid_ptr<AnyNode*> parent) : AnyNode(parent) {
        // properties
        for_each(m_node_properties, [this](auto& property) {
            using property_t = typename std::decay_t<decltype(property)>::element_type;
            property = std::make_unique<property_t>();
            if (property->is_visible()) { // receive an update, whenever a visible property changes its value
                property->get_operator()->subscribe(_get_property_observer());
            }
        });

        // slots
        for_each(m_node_slots, [this](auto& slot) {
            slot = std::make_unique<typename std::decay_t<decltype(slot)>::element_type>();
        });

        // signals
        for_each(m_node_signals, [this](auto& signal) {
            signal = std::make_shared<typename std::decay_t<decltype(signal)>::element_type>();
        });
    }

public:
    // properties -------------------------------------------------------------

    /// @{
    /// Returns the correctly typed value of a Property.
    /// @param name     Name of the requested Property.
    template<char... Cs>
    constexpr const auto& get(StringType<Cs...> name) const {
        return std::get<_get_index<NodeProperties>(name)>(m_node_properties)->get();
    }
    template<const ConstString& name>
    constexpr const auto& get() const {
        return get(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Updates the value of a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @param value    New value of the Property.
    template<class T, char... Cs, size_t I = _get_index<NodeProperties>(StringType<Cs...>{}),
             class = std::enable_if_t<std::is_convertible_v<T, typename node_property_t<I>::value_t>>>
    constexpr void set(StringType<Cs...>, T&& value) {
        std::get<I>(m_node_properties)->set(std::forward<T>(value));
    }
    template<const ConstString& name, class T>
    constexpr void set(T&& value) {
        set(make_string_type<name>(), std::forward<T>(value));
    }
    /// @}

    /// @{
    /// Returns the correctly typed value of a Property.
    /// @param name     Name of the requested Property.
    template<char... Cs>
    constexpr const auto& connect_property(StringType<Cs...> name) const {
        return PropertyHandle(std::get<_get_index<NodeProperties>(name)>(m_node_properties));
    }
    template<const ConstString& name>
    constexpr const auto& connect_property() const {
        return connect_property(make_string_type<name>());
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::connect_property;
    using AnyNode::get;
    using AnyNode::set;

    // signals / slots --------------------------------------------------------

    /// @{
    /// Manually call the requested Slot of this Node.
    /// If T is not`None`, this method takes a second argument that is passed to the Slot.
    /// The Publisher of the Slot's `on_next` call id is set to `nullptr`.
    template<char... Cs, size_t I = _get_index<NodeSlots>(StringType<Cs...>{}),
             class T = typename node_slot_t<I>::value_t>
    std::enable_if_t<std::is_same_v<T, None>> call(StringType<Cs...>) {
        std::get<I>(m_node_slots)->call();
    }
    template<char... Cs, size_t I = _get_index<NodeSlots>(StringType<Cs...>{}),
             class T = typename node_slot_t<I>::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(StringType<Cs...>, const T& value) {
        std::get<I>(m_node_slots)->call(value);
    }
    template<const ConstString& name, size_t I = _get_index<NodeSlots>(make_string_type<name>()),
             class T = typename node_slot_t<I>::value_t>
    std::enable_if_t<std::is_same_v<T, None>> call() {
        std::get<I>(m_node_slots)->call();
    }
    template<const ConstString& name, size_t I = _get_index<NodeSlots>(make_string_type<name>()),
             class T = typename node_slot_t<I>::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(const T& value) {
        std::get<I>(m_node_slots)->call(value);
    }
    /// @}

    /// @{
    /// Returns the requested Slot or void (which doesn't compile).
    /// @param name     Name of the requested Slot.
    template<char... Cs, size_t I = _get_index<NodeSlots>(StringType<Cs...>{})>
    constexpr auto connect_slot(StringType<Cs...>) const {
        return SlotHandle(std::get<I>(m_node_slots).get());
    }
    template<const ConstString& name>
    constexpr auto connect_slot() const {
        return connect_slot(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Returns the requested Signal or void (which doesn't compile).
    /// @param name     Name of the requested Signal.
    template<char... Cs, size_t I = _get_index<NodeSignals>(StringType<Cs...>{})>
    constexpr auto connect_signal(StringType<Cs...>) const {
        return std::get<I>(m_node_signals);
    }
    template<const ConstString& name>
    constexpr auto connect_signal() const {
        return connect_signal(make_string_type<name>());
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::connect_signal;
    using AnyNode::connect_slot;

protected:
    // properties -------------------------------------------------------------
    /// @{
    /// (Re-)Defines a callback to be invoked every time the value of the Property is about to change.
    template<char... Cs, size_t I = _get_index<NodeProperties>(StringType<Cs...>{})>
    constexpr void _set_property_callback(StringType<Cs...>, typename node_property_t<I>::callback_t callback) {
        std::get<I>(m_node_properties)->set_callback(std::move(callback));
    }
    template<const ConstString& name, size_t I = _get_index<NodeProperties>(make_string_type<name>())>
    constexpr void _set_property_callback(typename node_property_t<I>::callback_t callback) {
        _set_property_callback(make_string_type<name>(), std::move(callback));
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::_set_property_callback;

    // signals / slots --------------------------------------------------------

    /// @{
    /// Internal access to a Slot of this Node.
    /// @param name     Name of the requested Property.
    template<char... Cs, size_t I = _get_index<NodeSlots>(StringType<Cs...>{})>
    constexpr auto _get_slot(StringType<Cs...>) const {
        return std::get<I>(m_node_slots)->get_publisher();
    }
    template<const ConstString& name>
    constexpr auto _get_slot() const {
        return _get_slot(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Emits a Signal with a given value.
    /// @param name     Name of the Signal to emit.
    /// @param value    Data to emit.
    /// @throws         NameError / TypeError
    template<char... Cs, size_t I = _get_index<NodeSignals>(StringType<Cs...>{}),
             class T = typename node_signal_t<I>::value_t>
    void _emit(StringType<Cs...>, const T& value) {
        std::get<I>(m_node_signals)->publish(value);
    }
    template<char... Cs, size_t I = _get_index<NodeSignals>(StringType<Cs...>{}),
             class T = typename node_signal_t<I>::value_t>
    void _emit(StringType<Cs...>) {
        std::get<I>(m_node_signals)->publish();
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::_emit;
    using AnyNode::_get_slot;

protected: // TODO: maybe make private again and let the Widget use an Accessor instead?
    /// Implementation specific query of a Property.
    AnyProperty* _get_property_impl(const std::string& name) const override {
        if (const size_t index = _get_index<NodeProperties>(hash_string(name));
            index < std::tuple_size_v<NodeProperties>) {
            return visit_at<AnyProperty*>(m_node_properties, index,
                                          [](const auto& property) { return property.get(); });
        } else {
            return nullptr; // no such property
        }
    }

    /// Implementation specific query of a Slot, returns an empty pointer if no Slot by the given name is found.
    /// @param name     Node-unique name of the Slot.
    AnySlot* _get_slot_impl(const std::string& name) const override {
        if (const size_t index = _get_index<NodeSlots>(hash_string(name)); index < std::tuple_size_v<NodeSlots>) {
            return visit_at<AnySlot*>(m_node_slots, index, [](const auto& slot) { return slot.get(); });
        } else {
            return nullptr; // no such slot
        }
    }

    /// Implementation specific query of a Signal, returns an empty pointer if no Signal by the given name is found.
    /// @param name     Node-unique name of the Signal.
    AnySignalPtr _get_signal_impl(const std::string& name) const override {
        if (const size_t index = _get_index<NodeSignals>(hash_string(name)); index < std::tuple_size_v<NodeSignals>) {
            return visit_at<AnySignalPtr>(m_node_signals, index, [](const auto& signal) { return signal; });
        } else {
            return nullptr; // no such slot
        }
    }

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const override {
        for_each(
            m_node_properties, [](auto& property, size_t& out) { hash_combine(out, property->get()); }, result);
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() override {
        for_each(m_node_properties, [](auto& property) { property->clear_modified_data(); });
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    NodeProperties m_node_properties;

    /// All Slots of this Node.
    NodeSlots m_node_slots;

    /// All Signals of this Node.
    NodeSignals m_node_signals;
};

NOTF_CLOSE_NAMESPACE
