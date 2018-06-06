#pragma once

#include "app/property_graph.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

template<class T>
class GlobalProperty : public PropertyHead {

    using Dependencies = PropertyGraph::Dependencies;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of value of the Property.
    using type = T;

    /// Expression defining a Property of type T.
    using Expression = PropertyGraph::Expression<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value constructor.
    /// @param value    Initial value of the Property.
    GlobalProperty(T&& value) : PropertyHead(std::forward<T>(value)) {}

public:
    /// Factory method making sure that each GlobalProperty is managed by a shared_ptr.
    /// @param value    Initial value of the Property.
    static GlobalPropertyPtr<T> create(T&& value)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(GlobalProperty<T>, std::forward<T>(value));
    }

    /// The Property's value.
    const T& get() const { return _body().get(); }

    /// Sets the Property's value and fires a PropertyEvent.
    /// @param value        New value.
    void set(T&& value) { return _body().set(std::forward<T>(value)); }

    /// Set the Property's expression.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set(Expression expression, Dependencies dependencies)
    {
        _body().set(std::move(expression), std::move(dependencies));
    }

    /// Checks if the Property is grounded or not (has an expression).
    bool is_grounded() const { return _body().is_grounded(); }

    /// Checks if the Property has an expression or not (is grounded).
    bool has_expression() const { return _body().has_expression(); }

    /// Creates a TypedPropertyReader for this global property.
    TypedPropertyReader<T> reader() const
    {
        return TypedPropertyReader<T>(std::static_pointer_cast<TypedPropertyBody<T>>(m_body));
    }

private:
    /// The typed property body.
    TypedPropertyBody<T>& _body() const { return *(static_cast<TypedPropertyBody<T>*>(m_body.get())); }

    /// Updates the value in response to a PropertyEvent.
    /// @param update   PropertyUpdate to apply.
    void _apply_update(valid_ptr<PropertyUpdate*>) override
    {
        NOTF_NOOP; // a global property does not need to react to changes of its body
    }
};

/// Creates a global property with automatic type deduction.
/// @param value    Value of the new global property.
template<class T>
GlobalPropertyPtr<T> create_global_property(T&& value)
{
    return GlobalProperty<T>::create(std::forward<T>(value));
};

NOTF_CLOSE_NAMESPACE
