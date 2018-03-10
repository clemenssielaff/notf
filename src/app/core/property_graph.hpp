#pragma once

#include <atomic>
#include <cassert>
#include <unordered_map>

#include "app/core/property_key.hpp"
#include "common/any.hpp"
#include "common/exception.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

class PropertyGraph {

    // types -------------------------------------------------------------------------------------------------------//
public:
    /// Thrown when a Property with an unknown PropertyKey is requested.
    NOTF_EXCEPTION_TYPE(lookup_error)

    /// Thrown when an existing Property is asked for the wrong type of value.
    NOTF_EXCEPTION_TYPE(type_error)

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(cyclic_dependency_error)

private:
    ///
    /// Uses std::any for internal type erasure of the Property value.
    /// Type doubles as the conceptual `PropertyGroup` type, which is implicitly created by the PropertyGraph with the
    /// key: (ItemId, 0) whenever a Property from a new Item is being created.
    class Property {

        // methods ------------------------------------------------------------
    public:
        /// Default constructor.
        Property() = default;

        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        template<typename T>
        Property(const T& value)
        {
            m_value = value;
        }

        // group --------------------------------------------------------------

        /// Returns the next PropertyId in this group.
        PropertyId next_property_id()
        {
            assert(!m_value.has_value()); // group properties should not have a value
            return m_group_counter++;
        }

        /// Adds a new Property to this group.
        void add_property(Property* property)
        {
            assert(!m_value.has_value()); // group properties should not have a value
            m_affected.push_back(std::move(property));
        }

        /// All members of this group.
        const std::vector<Property*>& members() const { return m_affected; }

        // property -----------------------------------------------------------

        /// The property's value.
        /// If the property is defined by an expression, this might evaluate the expression.
        /// @param my_key       PropertyKey for error reporting.
        /// @param graph        Graph used to (potentially) evaluate the Property expression.
        /// @throws type_error  If the wrong value type is requested.
        /// @returns            Const reference to the value of the Property.
        template<typename T>
        const T& value(const PropertyKey& my_key, const PropertyGraph& graph)
        {
            _check_value_type(my_key, typeid(T));
            _clean_value(my_key, graph);

            const T* result = std::any_cast<T>(&m_value);
            assert(result);
            return *result;
        }

        /// Set the property's value.
        /// Removes an existing expression on the property if it exists.
        /// @param my_key       PropertyKey for error reporting.
        /// @param value        New value.
        /// @throws type_error  If a value of the wrong type is passed.
        template<typename T>
        void set_value(const PropertyKey& my_key, T&& value)
        {
            _check_value_type(my_key, typeid(T));
            _freeze();

            if (value != *std::any_cast<T>(&m_value)) {
                m_value = std::forward<T>(value);
                _set_affected_dirty();
            }
        }

        /// Set property's expression.
        /// @param my_key       PropertyKey for error reporting.
        /// @param expression   Expression to set.
        /// @param dependencies Properties that the expression depends on.
        /// @throws type_error  If an expression returning the wrong type is passed.
        template<typename T>
        void set_expression(const PropertyKey& my_key, std::function<T(const PropertyGraph&)> expression,
                            std::vector<Property*> dependencies)
        {
            _check_value_type(my_key, typeid(T));
            _clear_dependencies();

            // wrap the typed expression into one that returns a std::any
            m_expression
                = [expr = std::move(expression)](const PropertyGraph& graph) -> std::any { return expr(graph); };
            m_dependencies = std::move(dependencies);
            m_is_dirty = true;

            _register_with_dependencies();
            _set_affected_dirty();
        }

        /// All dependencies of this Property.
        const std::vector<Property*>& dependencies() const { return m_dependencies; }

        /// Prepares this Property for removal from the Graph.
        void prepare_removal()
        {
            _clear_dependencies();
            _freeze_affected();
        }

    private:
        /// Freezing a property means removing its expression without changing its value.
        void _freeze() noexcept
        {
            _clear_dependencies();
            m_expression = {};
            m_is_dirty = false;
        }

        /// Registers this property as affected with all of its dependencies.
        void _register_with_dependencies()
        {
            for (Property* dependency : m_dependencies) {
                dependency->m_affected.emplace_back(this);
            }
        }

        /// Removes all dependencies from this property.
        void _clear_dependencies() noexcept;

        /// Freezes all affected properties.
        void _freeze_affected()
        {
            for (Property* affected : m_affected) {
                affected->_freeze();
            }
        }

        /// Set all affected properties dirty.
        void _set_affected_dirty()
        {
            for (Property* affected : m_affected) {
                affected->m_is_dirty = true;
            }
        }

        /// Makes sure that the given type_info is the same as the one of the Property's value.
        /// @param my_key       PropertyKey for error reporting.
        /// @param info         Type to check.
        /// @throws type_error  If the wrong value type is requested.
        void _check_value_type(const PropertyKey& my_key, const std::type_info& info);

        /// Makes sure that the Property's value is clean.
        /// @param my_key   PropertyKey for error reporting.
        /// @param graph    Graph used to (potentially) evaluate the Property expression.
        void _clean_value(const PropertyKey& my_key, const PropertyGraph& graph);

        // fields -------------------------------------------------------------
    private:
        /// Value held by the Property.
        std::any m_value;

        /// Expression evaluating to a new value for this Property.
        std::function<std::any(const PropertyGraph&)> m_expression;

        /// All Properties that this one depends on.
        std::vector<Property*> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<Property*> m_affected;

        /// Counter used by PropertyGroups to generate new PropertyIds for members of the group.
        PropertyId::underlying_t m_group_counter{1};

        /// Whether the Property is dirty (its expression needs to be evaluated).
        bool m_is_dirty = false;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Checks if the graph has a Property with the given key.
    /// @param key  Key of the Property to search for.
    /// @returns    True iff there is a Property with the requested key in the graph.
    bool has_property(const PropertyKey& key) const { return m_properties.count(key) != 0; }

    /// @{
    /// Returns the value of the Property requested by type and key.
    /// @param key              Key of the requested Property.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    /// @return                 Value of the property.
    template<typename T>
    const T& value(const PropertyKey& key) const
    {
        const Property& property = _find_property(key);
        return property.value<T>(key, *this);
    }
    template<typename T>
    const T& value(const TypedPropertyKey<T> id) const
    {
        return value<T>(PropertyKey(id));
    }
    /// @}

    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param item_id  Item for which to construct the new Property.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @returns        Key of the new Property. Is invalid, if the given Item id is invalid.
    template<typename T>
    TypedPropertyKey<T> add_property(const ItemId item_id, const T& value)
    {
        if (!item_id.is_valid()) {
            return TypedPropertyKey<T>::invalid();
        }
        Property& group = _group_for(item_id);
        TypedPropertyKey<T> new_key(item_id, group.next_property_id());
        auto result = m_properties.emplace(std::make_pair(new_key, Property(value)));
        assert(result.second);
        group.add_property(&(*result.first));
        return new_key;
    }

    /// Sets the value of a Property.
    /// @param key              Key of the Property to modify.
    /// @param value            New value.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    template<typename T>
    void set_value(const PropertyKey& key, T&& value)
    {
        const Property& property = _find_property(key);
        property.set_value(key, std::forward<T>(value));
    }

    /// Sets the expression of a Property.
    /// It is of critical importance, that all properties in the expression are listed.
    /// @param key              Key of the Property to modify.
    /// @param expression       New expression defining the value of the property.
    /// @throws lookup_error    If a Property with the given id or one of the dependencies does not exist.
    /// @throws type_error      If the expression returns the wrong type.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    template<typename T>
    void set_expression(const PropertyKey& key, std::function<T(const PropertyGraph&)>&& expression,
                        const std::vector<PropertyKey>& dependency_keys)
    {
        const Property& property = _find_property(key);
        std::vector<Property*> dependencies = _collect_properties(dependency_keys);
        _detect_cycles(property, dependencies);
        property.set_expression(key, std::move(expression), std::move(dependency_keys));
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    void delete_property(const PropertyKey& key);

private:
    /// Property access with proper error reporting.
    /// @param key              Key of the Property to find.
    /// @throws lookup_error    If a property with the requested key does not exist in the graph.
    /// @returns                The requested Property.
    const Property& _find_property(const PropertyKey& key) const;

    /// Creates a new or returns an existing group Property for the given Item.
    /// @param item_id  PropertyGroups are identified by the ItemId alone.
    Property& _group_for(const ItemId item_id);

    /// Collects a list of properties from their keys.
    /// @param keys             All PropertyKeys.
    /// @throws lookup_error    If any PropertyKey refers to a Property that doesn't exist in this graph.
    std::vector<Property*> _collect_properties(const std::vector<PropertyKey>& keys);

    /// Detects potential cyclic dependencies in the graph before they are introduced.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    void _detect_cycles(const Property& property, const std::vector<Property*>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All properties in the graph.
    std::unordered_map<PropertyKey, Property> m_properties;
};

NOTF_CLOSE_NAMESPACE
