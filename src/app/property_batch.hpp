#pragma once

#include "app/property_graph.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class NOTF_NODISCARD PropertyBatch {

    template<class T>
    using Expression = PropertyGraph::Expression<T>;
    using Dependencies = PropertyGraph::Dependencies;
    using PropertyUpdateList = PropertyGraph::PropertyUpdateList;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(PropertyBatch);

    /// Empty default constructor.
    PropertyBatch() = default;

    /// Move Constructor.
    /// @param other    Other Batch to move from.
    PropertyBatch(PropertyBatch&& other) = default;

    /// Destructor.
    /// Tries to execute but will swallow any exceptions that might occur.
    /// In the case of an exception, the PropertyGraph will not be modified.
    ~PropertyBatch()
    {
        try {
            execute();
        }
        catch (...) {
            // ignore exceptions
        }
    }

    /// Set the Property's value and updates downstream Properties.
    /// Removes an existing expression on this Property if one exists.
    /// @param property     Property to update.
    /// @param value        New value.
    template<class T, class U = typename T::type>
    void set(T& property, U&& value)
    {
        static_assert(std::is_base_of<PropertyHead, T>::value, "T must be a subclass of PropertyHead");
        _set_value<U>(property, std::forward<U>(value));
    }

    /// Set the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param target           Property targeted by this update.
    /// @param expression       New expression for the targeted Property.
    /// @param dependencies     Property Readers that the expression depends on.
    template<class T, class U = typename T::type>
    void set(T& property, identity_t<Expression<U>>&& expression, Dependencies&& deps)
    {
        static_assert(std::is_base_of<PropertyHead, T>::value, "T must be a subclass of PropertyHead");
        _set_expression<U>(property, std::move(expression), std::move(deps));
    }

    /// Executes this batch.
    /// If any error occurs, this method will throw the exception and not modify the PropertyGraph.
    /// If no exception occurs, the transaction was successfull and the batch is empty again.
    /// @throws no_dag_error    If a Property expression update would cause a cyclic dependency in the graph.
    void execute();

private:
    /// Sets the Property's value.
    template<class T>
    void _set_value(PropertyHead& property, T&& value)
    {
        m_updates.emplace_back(std::make_unique<PropertyValueUpdate<T>>(
            PropertyHead::Access<PropertyBatch>::body(property), std::forward<T>(value)));
    }

    /// Sets the Property's expression.
    template<class T>
    void _set_expression(PropertyHead& property, identity_t<Expression<T>>&& expression, Dependencies&& dependencies)
    {
        m_updates.emplace_back(std::make_unique<PropertyExpressionUpdate<T>>(
            PropertyHead::Access<PropertyBatch>::body(property), std::move(expression), std::move(dependencies)));
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All updates that make up this batch.
    PropertyUpdateList m_updates;
};

NOTF_CLOSE_NAMESPACE
