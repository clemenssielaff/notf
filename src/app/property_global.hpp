#pragma once

#include "app/property_graph.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

template<class T>
class GlobalProperty : public PropertyHead<T> {

    using PropertyHead<T>::m_body;
    using Dependencies = PropertyGraph::Dependencies;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using Expression = PropertyGraph::Expression<T>;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value constructor.
    /// @param value    Initial value of the Property.
    GlobalProperty(T&& value) : PropertyHead<T>(std::forward<T>(value)) {}

public:
    /// Factory method making sure that each GlobalProperty is managed by a shared_ptr.
    /// @param value    Initial value of the Property.
    static GlobalPropertyPtr<T> create(T&& value)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(GlobalProperty<T>, std::forward<T>(value));
    }

    /// The Property's value.
    const T& value() const { return _body().value(); }

    /// Sets the Property's value and fires a PropertyEvent.
    /// @param value        New value.
    void set_value(T&& value) { return _body().set_value(std::forward<T>(value)); }

    /// Set the Property's expression.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(Expression expression, Dependencies dependencies)
    {
        _body().set_expression(std::move(expression), std::move(dependencies));
    }

    /// Checks if the Property is grounded or not (has an expression).
    bool is_grounded() const { return _body().is_grounded(); }

    /// Checks if the Property has an expression or not (is grounded).
    bool has_expression() const { return _body().has_expression(); }

    /// Creates a PropertyReader for this global property.
    PropertyReader<T> reader() const { return PropertyReader<T>(std::static_pointer_cast<PropertyBody<T>>(m_body)); }

private:
    /// The typed property body.
    PropertyBody<T>& _body() const { return *(static_cast<PropertyBody<T>*>(m_body.get())); }

    virtual void _apply_update(valid_ptr<PropertyUpdate*>) {}
};

/// Creates a global property with automatic type deduction.
/// @param value    Value of the new global property.
template<class T>
GlobalPropertyPtr<T> create_global_property(T&& value)
{
    return GlobalProperty<T>::create(std::forward<T>(value));
};

NOTF_CLOSE_NAMESPACE
