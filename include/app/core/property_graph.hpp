#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "app/io/time.hpp"
#include "common/exception.hpp"
#include "common/id.hpp"

namespace notf {

class PropertyGraph;

namespace property_graph_detail {
class PropertyBase;
} // namespace property_graph_detail

//====================================================================================================================//

/// Property id type.
using PropertyId = IdType<property_graph_detail::PropertyBase, size_t>;

/// Typed property id.
template<typename value_t>
using TypedPropertyId = IdType<PropertyId::type_t, PropertyId::underlying_t, value_t>;

//====================================================================================================================//

/// Exception when a property Id did not match a property in the graph.
NOTF_EXCEPTION_TYPE(property_lookup_error, "Failed to look up property by id")

/// Exception thrown when a new expression would introduce a cyclic dependency into the graph.
NOTF_EXCEPTION_TYPE(property_cyclic_dependency_error, "Failed to create property expression which would introduce a "
                                                      "cyclic dependency")

//====================================================================================================================//

namespace property_graph_detail {

#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

/// Base type of all properties.
class PropertyBase {
    friend class ::notf::PropertyGraph;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Underlying (numeric) property id type.
    using id_t = typename PropertyId::underlying_t;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param id       Property id.
    /// @param graph    Graph containing this property.
    PropertyBase(const id_t id, const PropertyGraph& graph)
        : m_id(id), m_graph(graph), m_is_dirty(false), m_time(Time::now()), m_dependencies(), m_affected()
    {}

    /// Destructor.
    virtual ~PropertyBase() noexcept;

    /// Id of this property.
    PropertyId id() const noexcept { return m_id; }

    /// Time when the property was set the last time.
    Time time() const noexcept { return m_time; }

protected: // for use by Property and PropertyGraph
    /// Freezing a property means removing its expression without changing its value.
    virtual void freeze() = 0;

    /// Removes all dependencies from this property.
    void clear_dependencies();

    /// Freezes all affected properties.
    void freeze_affected();

protected:
    /// Registers this property as affected with all of its dependencies.
    void _register_with_dependencies();

    /// Set all affected properties dirty.
    void _set_affected_dirty();

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// Property id.
    const id_t m_id;

    /// Graph containing this property.
    const PropertyGraph& m_graph;

    /// Whether the property is dirty (its expression needs to be evaluated).
    mutable bool m_is_dirty;

    /// Time when the property was set the last time.
    Time m_time;

    /// All properties that this one depends on.
    std::vector<PropertyBase*> m_dependencies;

    /// Properties affected by this one through expressions.
    std::vector<PropertyBase*> m_affected;
};

#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic pop
#endif

//====================================================================================================================//

/// A typed property with a value and mechanisms required for the property graph.
template<typename T>
class Property : public PropertyBase {
    friend class ::notf::PropertyGraph;

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// Value type.
    using value_t = T;

    /// Expression type.
    using expression_t = std::function<value_t(const PropertyGraph&)>;

    /// Underlying id type.
    using id_t = typename PropertyBase::id_t;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param id       Property id.
    /// @param graph    Graph containing this property.
    /// @param value    Property value.
    Property(const id_t id, const PropertyGraph& graph, value_t&& value)
        : PropertyBase(id, graph), m_expression(), m_value(std::forward<value_t>(value))
    {}

    /// Destructor.
    virtual ~Property() noexcept override {}

    /// The property's value.
    /// If the property is defined by an expression, this might evaluate the expression.
    const value_t& value() const
    {
        if (m_is_dirty) {
            assert(m_expression);
            m_value    = m_expression(m_graph);
            m_is_dirty = false;
        }
        return m_value;
    }

private: // for use by Property and PropertyGraph
    /// Set the property's value.
    /// Removes an existing expression on the property if it exists.
    /// @param value    New value.
    /// @param time     If the property's value is newer, it won't update. Is ignored if invalid (the default).
    void set_value(value_t&& value, Time time = {})
    {
        if (time) {
            if (time < m_time) {
                return;
            }
            m_time = std::move(time);
        }

        freeze();

        if (value != m_value) {
            m_value = std::forward<value_t>(value);
            _set_affected_dirty();
        }
    }

    /// Set property's expression.
    /// @param expression   Expression.
    /// @param dependencies Properties that the expression depends on.
    /// @param time         If the property's value is newer, it won't update. Is ignored if invalid (the default).
    void set_expression(expression_t expression, std::vector<PropertyBase*> dependencies, Time time = {})
    {
        if (time) {
            if (time < m_time) {
                return;
            }
            m_time = std::move(time);
        }

        clear_dependencies();

        m_expression   = std::move(expression);
        m_dependencies = std::move(dependencies);
        m_is_dirty     = true;

        _register_with_dependencies();
        _set_affected_dirty();
    }

    /// Freezing a property means removing its expression without changing its value.
    virtual void freeze() noexcept override final
    {
        clear_dependencies();
        m_expression = {};
        m_is_dirty   = false;
    }

    /// Expression evaluating to a new value for this property.
    expression_t m_expression;

    /// Property value.
    mutable value_t m_value;
};

} // namespace property_graph_detail

//====================================================================================================================//

/// The user is not expected to work with a PropertyGraph directly. Instead, all events in the system can CRUD (create,
/// read, update, delete) properties via a PropertyManager.
/// The only direct access is via Property<value_t>* pointers that are aquired via `add_property` or `property`.
/// Do not store these pointers! Only use them to create expressions that are passed back into the PropertyGraph.
class PropertyGraph {

    /// PropertyBase type.
    using PropertyBase = property_graph_detail::PropertyBase;

    /// Property type
    template<typename value_t>
    using Property = property_graph_detail::Property<value_t>;

    /// Underlying id type.
    using id_t = PropertyBase::id_t;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    PropertyGraph() : m_next_id(1), m_properties() {}

    /// Checks if the given id identifies a property of this graph.
    bool has_property(const PropertyId id) const { return m_properties.count(static_cast<id_t>(id)); }

    /// @{
    /// Returns as pointer to a property requested by type and id.
    /// @return Pointer to the property.
    /// @throws property_lookup_error   If a property with the given id does not exist or is of the wrong type.
    template<typename value_t>
    const value_t& property(const PropertyId id) const
    {
        const auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (const Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                return property->value();
            }
        }
        throw_notf_error(property_lookup_error);
    }
    template<typename value_t>
    const value_t& property(const TypedPropertyId<value_t> id) const
    {
        return property<value_t>(PropertyId(id));
    }
    /// @}

    /// Returns the next free property id.
    PropertyId next_id() { return m_next_id++; }

    /// Creates an new property with the given type and id.
    /// @param id               Id of the property.
    /// @throws runtime_error   If the property ID already exists.
    template<typename value_t>
    void add_property(const PropertyId id)
    {
        if (has_property(id)) {
            throw_notf_error_msg(property_lookup_error, "Cannot create a new property with an existing ID!");
        }
        const id_t numeric_id = static_cast<id_t>(id);
        const auto result
            = m_properties.emplace(numeric_id, std::make_unique<Property<value_t>>(numeric_id, *this, value_t{}));
        assert(result.second);
    }

    /// Define the value of a property identified by its id.
    /// @param expression   Expression.
    /// @param dependencies Properties that the expression depends on.
    /// @throws property_lookup_error   If a property with the given id does not exist or is of the wrong type.
    template<typename value_t>
    void set_property(const PropertyId id, value_t&& value, Time time = {})
    {
        auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                property->set_value(std::forward<value_t>(value), time);
                return;
            }
        }
        throw_notf_error(property_lookup_error);
    }

    /// Define the expression of a property identified by its id.
    /// It is of critical importance, that all properties in the expression are listed.
    /// @param id           Id of the property to set.
    /// @param expression   New value of the property.
    /// @param time         If the property's value is newer, it won't update. Is ignored if invalid (the default).
    /// @throws property_lookup_error
    ///                     If a property with the given id does not exist or is of the wrong type.
    /// @throws property_cyclic_dependency_error
    ///                     If the expression would introduce a cyclic dependency into the graph.
    template<typename value_t>
    void set_expression(const PropertyId id, std::function<value_t(const PropertyGraph&)> expression,
                        const std::vector<PropertyId>& dependencies, Time time = {})
    {
        auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                std::vector<PropertyBase*> dependent_properties;
                if (_get_properties(dependencies, dependent_properties)) {
                    if (_is_dependency_of_any(property, dependent_properties)) {
                        throw_notf_error(property_cyclic_dependency_error);
                    }
                    else {
                        property->set_expression(std::move(expression), std::move(dependent_properties), time);
                        return;
                    }
                }
            }
        }
        throw_notf_error(property_lookup_error);
    }

    /// Removes a property from the graph.
    /// All affected properties will have their value set to their current value.
    /// @warning    This deletes the property - all raw pointers to the property will become invalid immediately!
    /// @return     True, iff the id identifies a property in the graph.
    void delete_property(const PropertyId id);

private:
    /// Collects a list of properties from their ids.
    /// @param ids      All ids.
    /// @param result   All properties in this graph identified by an id.
    /// @returns        True if all ids refer to properties in this graph.
    bool _get_properties(const std::vector<PropertyId>& ids, std::vector<PropertyBase*>& result);

    /// Checks if the given property is a dependency of any of the given dependencies (or their dependencies etc.)
    /// Use this to make sure that new expressions do not introduce circular dependencies.
    /// @param property     Candidate property.
    /// @param dependencies All other properties to check for circular dependency.
    /// @return             True iff there is a circular dependency.
    bool _is_dependency_of_any(const PropertyBase* property, const std::vector<PropertyBase*>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Id counter to identify new properties.
    std::atomic<id_t> m_next_id;

    /// All properties, accessible by id.
    /// Uses an unordered map because we need the O(1) lookup and since `id_t` provides a perfect hash function, this
    /// should be the best option for a busy hashmap.
    std::unordered_map<id_t, std::unique_ptr<PropertyBase>> m_properties;
};

} // namespace notf
