#pragma once

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "common/exception.hpp"
#include "common/id.hpp"

namespace notf {

class PropertyGraph;

//====================================================================================================================//

namespace detail {

#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

/// Base type of all properties.
class PropertyBase {
    friend class ::notf::PropertyGraph;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Property id type.
    using id_t = uint;

    /// Property id type.
    using Id = IdType<PropertyBase, id_t>;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param id   Property id.
    PropertyBase(const id_t id) : m_id(id), m_is_dirty(false), m_dependencies(), m_affected() {}

    /// Destructor.
    virtual ~PropertyBase() noexcept;

    /// Id of this property.
    Id id() const { return m_id; }

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

    /// Whether the property is dirty (its expression needs to be evaluated).
    mutable bool m_is_dirty;

    /// All properties that this one depends on.
    std::vector<PropertyBase*> m_dependencies;

    /// Properties affected by this one through expressions.
    std::vector<PropertyBase*> m_affected;
};

#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic pop
#endif

} // namespace detail

//====================================================================================================================//

/// A typed property with a value and mechanisms required for the property graph.
template<typename T>
class Property : public detail::PropertyBase {
    friend class PropertyGraph;

    /// Value type.
    using value_t = T;

    /// Expression type.
    using expression_t = std::function<value_t()>;

    /// Underlying id type.
    using id_t = typename detail::PropertyBase::id_t;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param id           Property id.
    /// @param value        Property value.
    Property(const id_t id, value_t&& value)
        : detail::PropertyBase(id), m_expression(), m_value(std::forward<value_t>(value))
    {}

    /// Destructor.
    virtual ~Property() noexcept override {}

    /// The property's value.
    /// If the property is defined by an expression, this might evaluate the expression.
    const value_t& value() const
    {
        if (m_is_dirty) {
            assert(m_expression);
            m_value    = m_expression();
            m_is_dirty = false;
        }
        return m_value;
    }

private: // for use by Property and PropertyGraph
    /// Set the property's value.
    /// Removes an existing expression on the property if it exists.
    /// @param value    New value.
    void set_value(value_t&& value)
    {
        freeze();

        if (value != m_value) {
            m_value = std::forward<value_t>(value);
            _set_affected_dirty();
        }
    }

    /// Set property's expression.
    /// @param expression   Expression.
    /// @param dependencies Properties that the expression depends on.
    void set_expression(expression_t expression, std::vector<PropertyBase*> dependencies)
    {
        clear_dependencies();

        m_expression   = std::move(expression);
        m_dependencies = std::move(dependencies);
        m_is_dirty     = true;

        _register_with_dependencies();
        _set_affected_dirty();
    }

    /// Freezing a property means removing its expression without changing its value.
    virtual void freeze() override final
    {
        clear_dependencies();
        m_expression = {};
        m_is_dirty   = false;
    }

    /// Expression evaluating to a new value for this property.
    std::function<value_t()> m_expression;

    /// Property value.
    mutable value_t m_value;
};

//====================================================================================================================//

/// The user is not expected to work with a PropertyGraph directly. Instead, all events in the system can CRUD (create,
/// read, update, delete) properties via a PropertyManager.
/// The only direct access is via Property<value_t>* pointers that are aquired via `add_property` or `property`.
/// Do not store these pointers! Only use them to create expressions that are passed back into the PropertyGraph.
class PropertyGraph {

    /// PropertyBase type.
    using PropertyBase = detail::PropertyBase;

    /// Underlying id type.
    using id_t = PropertyBase::id_t;

    /// Typed id type.
    using Id = PropertyBase::Id;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    PropertyGraph() : m_next_id(1), m_properties() {}

    /// Checks if the given id identifies a property of this graph.
    bool has_property(const Id id) const { return m_properties.count(static_cast<id_t>(id)); }

    /// Creates an new property with the given type
    /// @param value    Value of the new property (can be left empty to zero initialize if the type is specified).
    /// @returns        Non-owning pointer to the new property.
    template<typename value_t>
    const Property<value_t>* add_property(value_t&& value = {})
    {
        const id_t id = _next_id();
        const auto result
            = m_properties.emplace(id, std::make_unique<Property<value_t>>(id, std::forward<value_t>(value)));
        assert(result.second);
        return static_cast<Property<value_t>*>(result.first->second.get());
    }

    /// Returns as pointer to a property requested by type and id.
    /// @return Risky pointer to the property. Is empty if a property with the given id does not exist or is of the
    /// wrong type.
    template<typename value_t>
    risky_ptr<const Property<value_t>> property(const Id id) const
    {
        const auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (const Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                return &property;
            }
        }
        return nullptr;
    }

    /// Define the value of a property identified by its id.
    /// @param expression   Expression.
    /// @param dependencies Properties that the expression depends on.
    /// @returns            True if the property was updated, false if the id does not identify a property or the
    /// property's type is wrong.
    template<typename value_t>
    bool set_property(const Id id, value_t&& value)
    {
        auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                property->set_value(std::forward<value_t>(value));
                return true;
            }
        }
        return false;
    }

    /// Define the expression of a property identified by its id.
    /// It is of critical importance, that all properties in the expression are listed.
    /// @param id       Id of the property to set.
    /// @param expression New value of the property.
    /// @returns        True if the property was updated, false if the id does not identify a property or the property's
    ///                 type is wrong.
    template<typename value_t>
    bool set_expression(const Id id, std::function<value_t()> expression, const std::vector<Id>& dependencies)
    {
        auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (Property<value_t>* property = dynamic_cast<Property<value_t>*>(it->second.get())) {
                std::vector<PropertyBase*> dependent_properties;
                if (_get_properties(dependencies, dependent_properties)) {
                    if (!_is_dependency_of_any(property, dependent_properties)) {
                        property->set_expression(std::move(expression), std::move(dependent_properties));
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /// Removes a property from the graph.
    /// All affected properties will have their value set to their current value.
    /// @warning    This deletes the property - all raw pointers to the property will become invalid immediately!
    /// @return     True, iff the id identifies a property in the graph.
    bool delete_property(const Id id);

private:
    /// Calculates and returns the next free property id.
    /// Even handles wrap-arounds if we should ever have more than size_t properties (highly! unlikely).
    /// Does not handle the case where you store more than size_t properties ... but by then, your computer has most
    /// likely long run out of memory anyway.
    id_t _next_id();

    /// Collects a list of properties from their ids.
    /// @param ids      All ids.
    /// @param result   All properties in this graph identified by an id.
    /// @returns        True if all ids refer to properties in this graph.
    bool _get_properties(const std::vector<Id>& ids, std::vector<PropertyBase*>& result);

    /// Checks if the given property is a dependency of any of the given dependencies (or their dependencies etc.)
    /// Use this to make sure that new expressions do not introduce circular dependencies.
    /// @param property     Candidate property.
    /// @param dependencies All other properties to check for circular dependency.
    /// @return             True iff there is a circular dependency.
    bool _is_dependency_of_any(const PropertyBase* property, const std::vector<PropertyBase*>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Id counter to identify new properties.
    id_t m_next_id;

    /// All properties, accessible by id.
    std::map<id_t, std::unique_ptr<PropertyBase>> m_properties;
};

} // namespace notf
