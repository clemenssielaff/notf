#pragma once

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "common/exception.hpp"
#include "common/id.hpp"

namespace notf {

//====================================================================================================================//

class PropertyGraph {

    /// Property id type.
    using id_t = size_t;

    // types ---------------------------------------------------------------------------------------------------------//
#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
    /// Base type of all properties.
    struct PropertyBase {
        /// Constructor.
        /// @param id   Property id.
        PropertyBase(const id_t id) : m_id(id), m_dependencies(), m_affected(), m_is_dirty(false) {}

        /// Destructor.
        virtual ~PropertyBase() noexcept;

        /// Freezing a property means removing its expression without changing its value.
        virtual void freeze() = 0;

        /// All properties that this one depends on.
        const std::vector<PropertyBase*> dependencies() const { return m_dependencies; }

        /// Removes all dependencies from this property.
        void clear_dependencies();

        /// Registers this property as affected with all of its dependencies.
        void register_with_dependencies();

        /// Set all affected properties dirty.
        void set_affected_dirty();

        /// Freezes all affected properties.
        void freeze_affected();

    protected:
        /// Property id.
        const id_t m_id;

        /// All properties that this one depends on.
        std::vector<PropertyBase*> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<PropertyBase*> m_affected;

        /// Whether the property is dirty (its expression needs to be evaluated).
        mutable bool m_is_dirty;
    };

    /// A typed property with a value and mechanisms required for the property graph.
    template<typename T>
    struct TypedProperty : public PropertyBase {

        /// Value type.
        using value_t = T;

        /// Expression type.
        using expression_t = std::function<value_t()>;

        /// Constructor.
        /// @param id           Property id.
        /// @param value        Property value.
        TypedProperty(const id_t id, value_t&& value)
            : PropertyBase(id), m_expression(), m_value(std::forward<value_t>(value))
        {}

        /// Destructor.
        virtual ~TypedProperty() noexcept override {}

        /// Freezing a property means removing its expression without changing its value.
        virtual void freeze() override final
        {
            clear_dependencies();
            m_expression = {};
            m_is_dirty   = false;
        }

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

        /// Set the property's value.
        /// Removes an existing expression on the property if it exists.
        /// @param value    New value.
        void set_value(value_t&& value)
        {
            freeze();

            if (value != m_value) {
                m_value = std::forward<value_t>(value);
                set_affected_dirty();
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

            register_with_dependencies();
            set_affected_dirty();
        }

    private:
        /// Expression evaluating to a new value for this property.
        std::function<value_t()> m_expression;

        /// Property value.
        mutable value_t m_value;
    };

#if defined(NOTF_CLANG) || defined(NOTF_IDE)
#pragma clang diagnostic pop
#endif

public:
    /// Property id type.
    using Id = IdType<PropertyBase, id_t>;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    PropertyGraph() : m_next_id(1), m_properties() {}

    /// Checks if the given id identifies a property of this graph.
    bool has_property(const Id id) const { return m_properties.count(static_cast<id_t>(id)); }

    /// Creates an new property with the given type
    template<typename value_t>
    Id add_property(value_t&& value = {})
    {
        const id_t id = _next_id();
        const auto result
            = m_properties.emplace(id, std::make_unique<TypedProperty<value_t>>(id, std::forward<value_t>(value)));
        assert(result.second);
        return id;
    }

    /// Returns the value of a typed property requested by type and id.
    /// @return Risky pointer to the result (may be empty) if the property does not exist or is of the wrong type.
    template<typename value_t>
    risky_ptr<const value_t> property(const Id id) const
    {
        const auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (const TypedProperty<value_t>* property = dynamic_cast<TypedProperty<value_t>*>(it->second.get())) {
                return &property->value();
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
            if (TypedProperty<value_t>* property = dynamic_cast<TypedProperty<value_t>*>(it->second.get())) {
                property->set_value(std::forward<value_t>(value));
                return true;
            }
        }
        return false;
    }

    /// Define the expression of a property identified by its id.
    /// @param id       Id of the property to set.
    /// @param expression New value of the property.
    /// @returns        True if the property was updated, false if the id does not identify a property or the property's
    ///                 type is wrong.
    template<typename value_t>
    bool set_expression(const Id id, std::function<value_t()> expression, const std::vector<Id>& dependencies)
    {
        auto it = m_properties.find(static_cast<id_t>(id));
        if (it != m_properties.end()) {
            if (TypedProperty<value_t>* property = dynamic_cast<TypedProperty<value_t>*>(it->second.get())) {
                std::vector<PropertyBase*> dependent_properties;
                if (_get_properties(dependencies, dependent_properties)) {
                    if (!_is_dependency_of_any(property, dependencies)) {
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
    bool _is_dependency_of_any(const PropertyBase* property, const std::vector<const PropertyBase*>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Atomic id counter to identify new properties.
    id_t m_next_id;

    /// All properties, accessible by id.
    std::map<id_t, std::unique_ptr<PropertyBase>> m_properties;
};

} // namespace notf
