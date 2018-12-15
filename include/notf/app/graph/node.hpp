#pragma once

#include "notf/app/graph/any_node.hpp"
#include "notf/app/graph/property.hpp"

NOTF_OPEN_NAMESPACE

// node policy factory ============================================================================================== //

namespace detail {

template<class Policy>
class NodePolicyFactory { // TODO: (NodePolicyFactory) check that all names are unique

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

    NOTF_CREATE_TYPE_DETECTOR(properties);
    static constexpr auto create_properties() {
        if constexpr (has_properties_v<Policy>) {
            return std::declval<typename instantiate_unique<Property, typename Policy::properties>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(slots);
    static constexpr auto create_slots() {
        if constexpr (has_slots_v<Policy>) {
            return std::declval<typename instantiate_unique<Slot, typename Policy::slots>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(signals);
    static constexpr auto create_signals() {
        if constexpr (has_signals_v<Policy>) {
            return std::declval<typename instantiate_shared<Signal, typename Policy::signals>::type>();
        } else {
            return std::tuple<>();
        }
    }

public:
    /// Factory method.
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

// compile time node ================================================================================================ //

template<class Policy = detail::EmptyNodePolicy>
class Node : public AnyNode {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Node type.
    using policy_t = typename detail::NodePolicyFactory<Policy>::NodePolicy;

protected:
    /// Tuple of Property types managed by this Node.
    using Properties = typename policy_t::properties;

    /// Type of a specific Property of this Node.
    template<size_t I>
    using property_t = typename std::tuple_element_t<I, Properties>::element_type;

    /// Tuple of Slot types managed by this Node.
    using Slots = typename policy_t::slots;

    /// Type of a specific Slot of this Node.
    template<size_t I>
    using slot_t = typename std::tuple_element_t<I, Slots>::element_type;

    /// Tuple of Signals types managed by this Node.
    using Signals = typename policy_t::signals;

    /// Type of a specific Signal of this Node.
    template<size_t I>
    using signal_t = typename std::tuple_element_t<I, Signals>::element_type;

    // helper --------------------------------------------------------------------------------- //
private:
    /// Finds the index of a Property by its name.
    template<size_t I = 0, char... Cs>
    static constexpr size_t _get_property_index(StringType<Cs...> name) {
        if constexpr (I < std::tuple_size_v<Properties>) {
            if constexpr (property_t<I>::get_const_name() == name) {
                return I;
            } else {
                return _get_property_index<I + 1>(name);
            }
        } else {
            NOTF_THROW(OutOfBounds);
        }
    }

    /// Finds the index of a Slot by its name.
    template<size_t I = 0, char... Cs>
    static constexpr size_t _get_slot_index(StringType<Cs...> name) {
        if constexpr (I < std::tuple_size_v<Slots>) {
            if constexpr (slot_t<I>::get_const_name() == name) {
                return I;
            } else {
                return _get_slot_index<I + 1>(name);
            }
        } else {
            NOTF_THROW(OutOfBounds);
        }
    }

    /// Finds the index of a Signal by its name.
    template<size_t I = 0, char... Cs>
    static constexpr size_t _get_signal_index(StringType<Cs...> name) {
        if constexpr (I < std::tuple_size_v<Signals>) {
            if constexpr (signal_t<I>::get_const_name() == name) {
                return I;
            } else {
                return _get_signal_index<I + 1>(name);
            }
        } else {
            NOTF_THROW(OutOfBounds);
        }
    }

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Node(valid_ptr<AnyNode*> parent) : AnyNode(parent) {
        // properties
        for_each(m_properties, [this](auto& property) {
            using property_t = typename std::decay_t<decltype(property)>::element_type;

            // create the new property
            property = std::make_unique<property_t>();

            // subscribe to receive an update, whenever a visible property changes its value
            if (property->is_visible()) { property->get_operator()->subscribe(_get_property_observer()); }
        });

        // slots
        for_each(m_slots, [this](auto& slot) {
            slot = std::make_unique<typename std::decay_t<decltype(slot)>::element_type>();
        });

        // signals
        for_each(m_signals, [this](auto& signal) {
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
        return std::get<_get_property_index(name)>(m_properties)->get();
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
    template<class T, char... Cs, size_t I = _get_property_index(StringType<Cs...>{}),
             class = std::enable_if_t<std::is_convertible_v<T, typename property_t<I>::value_t>>>
    constexpr void set(StringType<Cs...>, T&& value) {
        std::get<I>(m_properties)->set(std::forward<T>(value));
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
        return PropertyHandle(std::get<_get_property_index(name)>(m_properties));
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
    template<char... Cs, size_t I = _get_slot_index(StringType<Cs...>{}), class T = typename slot_t<I>::value_t>
    std::enable_if_t<std::is_same_v<T, None>> call(StringType<Cs...>) {
        std::get<I>(m_slots)->call();
    }
    template<char... Cs, size_t I = _get_slot_index(StringType<Cs...>{}), class T = typename slot_t<I>::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(StringType<Cs...>, const T& value) {
        std::get<I>(m_slots)->call(value);
    }
    template<const ConstString& name, size_t I = _get_slot_index(make_string_type<name>()),
             class T = typename slot_t<I>::value_t>
    std::enable_if_t<std::is_same_v<T, None>> call() {
        std::get<I>(m_slots)->call();
    }
    template<const ConstString& name, size_t I = _get_slot_index(make_string_type<name>()),
             class T = typename slot_t<I>::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(const T& value) {
        std::get<I>(m_slots)->call(value);
    }
    /// @}

    /// @{
    /// Returns the requested Slot or void (which doesn't compile).
    /// @param name     Name of the requested Slot.
    template<char... Cs, size_t I = _get_slot_index(StringType<Cs...>{})>
    constexpr auto connect_slot(StringType<Cs...>) const {
        return SlotHandle(std::get<I>(m_slots).get());
    }
    template<const ConstString& name>
    constexpr auto connect_slot() const {
        return connect_slot(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Returns the requested Signal or void (which doesn't compile).
    /// @param name     Name of the requested Signal.
    template<char... Cs, size_t I = _get_signal_index(StringType<Cs...>{})>
    constexpr auto connect_signal(StringType<Cs...>) const {
        return std::get<I>(m_signals);
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
    template<char... Cs, size_t I = _get_property_index(StringType<Cs...>{})>
    constexpr void _set_property_callback(StringType<Cs...>, typename property_t<I>::callback_t callback) {
        std::get<I>(m_properties)->set_callback(std::move(callback));
    }
    template<const ConstString& name, size_t I = _get_property_index(make_string_type<name>())>
    constexpr void _set_property_callback(typename property_t<I>::callback_t callback) {
        _set_property_callback(make_string_type<name>(), std::move(callback));
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::_set_property_callback;

    // signals / slots --------------------------------------------------------

    /// @{
    /// Internal access to a Slot of this Node.
    /// @param name     Name of the requested Property.
    template<char... Cs, size_t I = _get_slot_index(StringType<Cs...>{})>
    constexpr auto _get_slot(StringType<Cs...>) const {
        return std::get<I>(m_slots)->get_publisher();
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
    template<char... Cs, size_t I = _get_signal_index(StringType<Cs...>{}), class T = typename signal_t<I>::value_t>
    void _emit(StringType<Cs...>, const T& value) {
        std::get<I>(m_signals)->publish(value);
    }
    template<char... Cs, size_t I = _get_signal_index(StringType<Cs...>{}), class T = typename signal_t<I>::value_t>
    void _emit(StringType<Cs...>) {
        std::get<I>(m_signals)->publish();
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using AnyNode::_emit;
    using AnyNode::_get_slot;

private:
    /// @{
    /// Implementation specific query of a Property.
    AnyProperty* _get_property_impl(const std::string& name) const final { return _get_property_ct(hash_string(name)); }
    template<size_t I = 0>
    AnyProperty* _get_property_ct(const size_t hash_value) const {
        if constexpr (I < std::tuple_size_v<Properties>) {
            if (property_t<I>::get_const_name().get_hash() == hash_value) {
                return std::get<I>(m_properties).get();
            } else {
                return _get_property_ct<I + 1>(hash_value);
            }
        } else {
            return {}; // no such property
        }
    }
    /// @}

    /// @{
    /// Implementation specific query of a Slot, returns an empty pointer if no Slot by the given name is found.
    /// @param name     Node-unique name of the Slot.
    AnySlot* _get_slot_impl(const std::string& name) const final { return _get_slot_ct(hash_string(name)); }
    template<size_t I = 0>
    AnySlot* _get_slot_ct(const size_t hash_value) const {
        if constexpr (I < std::tuple_size_v<Slots>) {
            if (slot_t<I>::get_const_name().get_hash() == hash_value) {
                return std::get<I>(m_slots).get();
            } else {
                return _get_slot_ct<I + 1>(hash_value);
            }
        } else {
            return {}; // no such slot
        }
    }
    /// @}

    /// @{
    /// Implementation specific query of a Signal, returns an empty pointer if no Signal by the given name is found.
    /// @param name     Node-unique name of the Signal.
    AnySignalPtr _get_signal_impl(const std::string& name) const final { return _get_signal_ct(hash_string(name)); }
    template<size_t I = 0>
    AnySignalPtr _get_signal_ct(const size_t hash_value) const {
        if constexpr (I < std::tuple_size_v<Signals>) {
            if (signal_t<I>::get_const_name().get_hash() == hash_value) {
                return std::get<I>(m_signals);
            } else {
                return _get_signal_ct<I + 1>(hash_value);
            }
        } else {
            return {}; // no such signal
        }
    }
    /// @}

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const final {
        for_each(m_properties, [](auto& property, size_t& out) { hash_combine(out, property->get()); }, result);
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() final {
        for_each(m_properties, [](auto& property) { property->clear_modified_data(); });
    }

    // hide some protected methods
    using AnyNode::_get_property_observer;
    using AnyNode::_is_finalized;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    Properties m_properties;

    /// All Slots of this Node.
    Slots m_slots;

    /// All Signals of this Node.
    Signals m_signals;
};

NOTF_CLOSE_NAMESPACE
