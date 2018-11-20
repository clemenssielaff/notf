#pragma once

#include <map>

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// run time node ==================================================================================================== //

class RunTimeNode : public Node {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    RunTimeNode(valid_ptr<Node*> parent) : Node(parent) {}

    /// Implementation specific query of a Property.
    AnyPropertyPtr _get_property(const std::string& name) const override
    {
        if (auto itr = m_properties.find(name); itr != m_properties.end()) { return itr->second; }
        return {};
    }

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const override
    {
        for (auto& itr : m_properties) {
            hash_combine(result, itr.second->get_hash());
        }
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() override
    {
        for (auto& itr : m_properties) {
            itr.second->clear_modified_data();
        }
    }

    /// Constructs a new Property on this Node.
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @param is_visible   Whether or not the Property is visible.
    /// @throws FinalizedError  If you call this method from anywhere but the constructor.
    /// @throws not_unique_error    If there already exists a Property of the same name on this Node.
    template<class T>
    PropertyHandle<T> _create_property(std::string name, T&& value, const bool is_visible = true)
    {
        if (_is_finalized()) {
            NOTF_THROW(FinalizedError, "Cannot create a new Property on the finalized Node \"{}\"", get_name());
        }
        if (m_properties.count(name) != 0) {
            NOTF_THROW(NotUniqueError, "Node \"{}\" already has a Property named \"{}\"", get_name(), name);
        }

        // create an empty pointer first, to establish the name in the map
        auto [itr, success] = m_properties.emplace(std::move(name), nullptr);
        NOTF_ASSERT(success);

        // replace the empty pointer with the actual RunTimeProperty and create a string_view from the existing name
        PropertyPtr<T> property = std::make_shared<RunTimeProperty<T>>(itr->first, std::forward<T>(value), is_visible);
        itr->second = property;

        // subscribe to receive an update, whenever the property changes its value
        property->get_operator()->subscribe(_get_property_observer());

        return PropertyHandle<T>(std::move(property));
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Dynamically typed Properties.
    std::map<std::string, AnyPropertyPtr> m_properties;
};

NOTF_CLOSE_NAMESPACE
