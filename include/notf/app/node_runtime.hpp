#pragma once

#include <map>

#include "notf/app/node.hpp"
#include "notf/app/property_runtime.hpp"

NOTF_OPEN_NAMESPACE

// run time node ==================================================================================================== //

class RunTimeNode : public Node {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    RunTimeNode(valid_ptr<Node*> parent) : Node(parent) {}

    /// Constructs a new Property on this Node.
    /// Note that the `get_node` method on the returned PropertyHandle returns an expired NodeHandle until the Node is
    /// finalized (the constructor has finished).
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @param is_visible   Whether or not the Property is visible.
    /// @throws FinalizedError      If you call this method from anywhere but the constructor.
    /// @throws not_unique_error    If there already exists a Property of the same name on this Node.
    /// @returns            A Handle to the created Property.
    template<class T>
    PropertyHandle<T> _create_property(std::string name, T&& value, const bool is_visible = true) {
        if (NOTF_UNLIKELY(_is_finalized())) { // unlikely 'cause you only do it once
            NOTF_THROW(FinalizedError, "Cannot create a new Property on already finalized Node \"{}\"", get_name());
        }
        if (NOTF_UNLIKELY(m_properties.count(name) != 0)) {
            NOTF_THROW(NotUniqueError, "Node \"{}\" already has a Property named \"{}\"", get_name(), name);
        }

        // create an empty pointer first, to establish the name in the map
        auto [itr, success] = m_properties.emplace(std::move(name), nullptr);
        NOTF_ASSERT(success);

        // create the RunTimeProperty and store a string_view to the existing name in the map
        PropertyPtr<T> property = std::make_shared<RunTimeProperty<T>>(itr->first, std::forward<T>(value), is_visible);

        // replace the empty pointer in the map with the actual RunTimeProperty
        itr->second = property;

        // subscribe to receive an update, whenever the property changes its value
        property->get_operator()->subscribe(_get_property_observer());

        return property;
    }

    /// Constructs a new Slot on this Node.
    /// @param name         Name of the Slot.
    /// @throws FinalizedError      If you call this method from anywhere but the constructor.
    /// @throws not_unique_error    If there already exists a Slot of the same name on this Node.
    /// @returns            The internal publisher of the Slot.
    template<class T>
    typename Slot<T>::publisher_t _create_slot(std::string name) {
        if (NOTF_UNLIKELY(_is_finalized())) { // unlikely 'cause you only do it once
            NOTF_THROW(FinalizedError, "Cannot create a new Slot on already finalized Node \"{}\"", get_name());
        }
        if (NOTF_UNLIKELY(m_slots.count(name) != 0)) {
            NOTF_THROW(NotUniqueError, "Node \"{}\" already has a Slot named \"{}\"", get_name(), name);
        }

        auto [itr, success] = m_slots.emplace(std::move(name), std::make_unique<Slot<T>>());
        NOTF_ASSERT(success);

        return itr->second->get_publisher();
    }

    /// Constructs a new Signal on this Node.
    /// @param name         Name of the Signal.
    /// @throws FinalizedError      If you call this method from anywhere but the constructor.
    /// @throws not_unique_error    If there already exists a Signal of the same name on this Node.
    template<class T>
    void _create_signal(std::string name) {
        if (NOTF_UNLIKELY(_is_finalized())) { // unlikely 'cause you only do it once
            NOTF_THROW(FinalizedError, "Cannot create a new Signal on already finalized Node \"{}\"", get_name());
        }
        if (NOTF_UNLIKELY(m_signals.count(name) != 0)) {
            NOTF_THROW(NotUniqueError, "Node \"{}\" already has a Signal named \"{}\"", get_name(), name);
        }

        auto [itr, success] = m_signals.emplace(std::move(name), std::make_shared<Signal<T>>());
        NOTF_ASSERT(success);
    }

private:
    /// Implementation specific query of a Property.
    AnyPropertyPtr _get_property_impl(const std::string& name)  const final {
        if (auto itr = m_properties.find(name); itr != m_properties.end()) { return itr->second; }
        return {};
    }

    /// Implementation specific query of a Slot.
    AnySlot* _get_slot_impl(const std::string& name) const final {
        if (auto itr = m_slots.find(name); itr != m_slots.end()) { return itr->second.get(); }
        return {};
    }

    /// Implementation specific query of a Signal.
    AnySignalPtr _get_signal_impl(const std::string& name) const final {
        if (auto itr = m_signals.find(name); itr != m_signals.end()) { return itr->second; }
        return {};
    }

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const final {
        for (auto& itr : m_properties) {
            hash_combine(result, itr.second->get_hash());
        }
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() final {
        for (auto& itr : m_properties) {
            itr.second->clear_modified_data();
        }
    }

    /// Finalizes this Node.
    /// Stores a Handle of this node in all of its Properties.
    void _finalize() final {
        for (auto& itr : m_properties) {
            AnyProperty::AccessFor<Node>::set_node(*itr.second, shared_from_this());
        }
        Node::_finalize();
    }

    // hide some protected methods
    using Node::_finalize;
    using Node::_get_property_observer;
    using Node::_is_finalized;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Dynamically typed Properties.
    std::map<std::string, AnyPropertyPtr> m_properties;

    /// Slots.
    std::map<std::string, AnySlotPtr> m_slots;

    /// Signals.
    std::map<std::string, AnySignalPtr> m_signals;
};

NOTF_CLOSE_NAMESPACE
