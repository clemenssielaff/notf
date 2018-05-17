#pragma once

#include <functional>
#include <set>

#include "app/forwards.hpp"
#include "common/assert.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace temp {

//====================================================================================================================//

class PropertyGraph {

    // types ---------------------------------------------------------------------------------------------------------//
private:
    struct Update;

public:
    template<class, class>
    class PropertyBody;
    class PropertyBodyBase;

    template<class>
    class PropertyAccessor;
    class PropertyAccessorBase;

    class Batch;

    // ------------------------------------------------------------------------
public:
    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag);

    // ------------------------------------------------------------------------
public:
    /// Checks if T is a valid type to store in a Property.
    template<class T>
    using is_property_type = typename std::conjunction< //
        std::is_copy_constructible<T>,                  // we need to copy property values to store them as deltas
        std::negation<std::is_const<T>>>;               // property values must be modifyable
    template<class T>
    static constexpr bool is_property_type_v = is_property_type<T>::value;

    // ------------------------------------------------------------------------
public:
    /// Expression defining a Property of type T.
    template<class T>
    using Expression = std::function<T()>;

    /// Validator function for a Property of type T.
    template<class T>
    using Validator = std::function<bool(T&)>;

    /// Owning pointer to an untyped PropertyBody.
    using PropertyBodyPtr = std::shared_ptr<PropertyBodyBase>;

    /// Owning pointer to a typed PropertyBody.
    template<class T>
    using TypedPropertyBodyPtr = std::shared_ptr<PropertyBody<T, void>>;

    /// Set of update to untyped properties.
    using UpdateSet = std::set<std::unique_ptr<Update>>;

    //=========================================================================
private:
    /// Virtual baseclass, so we can store updates of various property types in one `UpdateSet`.
    struct Update {

        /// Constructor.
        /// @param target   Property targeted by this update.
        Update(PropertyBodyPtr target) : property(std::move(target)) {}

        /// Destructor.
        virtual ~Update();

        /// Property that was updated.
        PropertyBodyPtr property;
    };

    /// Stores a single value update for a property.
    template<class T>
    struct ValueUpdate final : public Update {

        /// Constructor.
        /// @param target   Property targeted by this update.
        /// @param value    New value of the targeted Property.
        ValueUpdate(PropertyBodyPtr target, T&& value) : Update(std::move(target)), value(std::forward<T>(value)) {}

        /// New value of the targeted Property.
        T value; // not const so we can move from it
    };

    /// Stores an expression update for a property.
    template<class T>
    struct ExpressionUpdate final : public Update {

        /// Constructor.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Accessors of Properties that the expression depends on.
        ExpressionUpdate(PropertyBodyPtr target, Expression<T>&& expression,
                         std::vector<PropertyAccessorBase>&& dependencies)
            : Update(std::move(target))
            , expression(std::forward<Expression<T>>(expression))
            , dependencies(std::move(dependencies))
        {}

        /// New value of the targeted Property.
        Expression<T> expression; // not const so we can move from it

        /// Properties that the expression depends on.
        std::vector<PropertyAccessorBase> dependencies; // not const so we can move from it
    };

    //=========================================================================
public:
    class NOTF_NODISCARD Batch {

        // methods ------------------------------------------------------------
    public:
        NOTF_NO_COPY_OR_ASSIGN(Batch);

        /// Move Constructor.
        /// @param other    Other Batch to move from.
        Batch(Batch&& other) : m_updates(std::move(other.m_updates)) {}

        /// Destructor.
        /// Tries to execute but will swallow any exceptions that might occur.
        /// In the case of an exception, the PropertyGraph will not be modified.
        ~Batch()
        {
            try {
                execute();
            }
            catch (...) {
                // ignore
            }
        }

        /// Set the Property's value and updates downstream Properties.
        /// Removes an existing expression on this Property if one exists.
        /// @param property     Property to update.
        /// @param value        New value.
        template<class T>
        void set_value(PropertyPtr<T> property, T&& value)
        {
            m_updates.emplace(std::make_unique<ValueUpdate<T>>(std::move(property), std::forward<T>(value)));
        }

        /// Set the Property's expression.
        /// Evaluates the expression right away to update the Property's value.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Accessors of Properties that the expression depends on.
        template<class T>
        void set_expression(PropertyPtr<T> property, identity_t<Expression<T>>&& expression,
                            std::vector<PropertyAccessorBase>&& dependencies)
        {
            m_updates.emplace(std::make_unique<ExpressionUpdate<T>>(
                std::move(property), std::forward<Expression<T>>(expression), std::move(dependencies)));
        }

        /// Executes this batch.
        /// If any error occurs, this method will throw the exception and not modify the PropertyGraph.
        /// If no exception occurs, the transaction was successfull and the batch is empty again.
        /// @throws no_dag  If a Property expression update would cause a cyclic dependency in the graph.
        void execute();

        // fields -------------------------------------------------------------
    private:
        /// All updates that make up this batch.
        UpdateSet m_updates;
    };

    //=========================================================================
public:
    class PropertyHead {};

    //=========================================================================
public:
    class PropertyBodyBase {

        template<class, class>
        friend class PropertyBody;

        // types --------------------------------------------------------------
    protected:
        /// Owning references to all PropertyBodies that this one depends on through its expression.
        using Dependencies = std::vector<PropertyAccessorBase>;

        /// Set of all property bodies affected by a change in the graph.
        using Affected = std::set<valid_ptr<const PropertyBodyBase*>>;

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        virtual ~PropertyBodyBase();

    protected:
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        virtual void _update(Affected& all_affected) = 0;

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        virtual void _ground();

        /// Tests whether the propposed upstream can be accepted, or would introduce a cyclic dependency.
        /// @param dependencies Dependencies to test.
        /// @throws no_dag      If the connections would introduce a cyclic dependency into the graph.
        void _test_upstream(const Dependencies& dependencies);

        /// Updates the upstream properties that this one depends on through its expression.
        /// @throws no_dag  If the connections would introduce a cyclic dependency into the graph.
        void _set_upstream(Dependencies&& dependencies);

        /// Adds a new downstream property that is affected by this one through an expression.
        void _add_downstream(const valid_ptr<PropertyBodyBase*> affected);

        // fields -------------------------------------------------------------
    protected:
        /// Owning references to all PropertyBodies that this one depends on through its expression.
        Dependencies m_upstream;

        /// PropertyBodies depending on this through their expressions.
        std::vector<valid_ptr<PropertyBodyBase*>> m_downstream;
    };

    template<class T, typename = std::enable_if_t<is_property_type_v<T>>>
    class PropertyBody : public PropertyBodyBase {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the property type.
        PropertyBody(T&& value) : PropertyBodyBase(), m_value(std::forward<T>(value)) {}

        /// Checks if the Property is grounded or not (has an expression).
        bool is_grounded() const
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());
            return !static_cast<bool>(m_expression);
        }

        /// The Property's value.
        const T& value() const
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());
            return m_value;
        }

        /// Set the Property's value and updates downstream Properties.
        /// Removes an existing expression on this Property if one exists.
        /// @param value            New value.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        void set_value(T&& value, Affected& all_affected)
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());
            if (m_expression) {
                _ground();
            }
            _set_value(std::forward<T>(value), all_affected);
        }

        /// Set the Property's expression.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
        void set_expression(Expression<T> expression, Dependencies dependencies, Affected& all_affected)
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // always remove the current expression, even if the new one might be invalid
            _ground();

            if (expression) {
                // update connections on yourself and upstream
                _set_upstream(std::move(dependencies)); // might throw no_dag
                m_expression = std::move(expression);

                // update the value of yourself and all downstream properties
                _update(all_affected);
            }
        }

    private:
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        void _update(Affected& all_affected) override
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            if (m_expression) {
                _set_value(m_expression(), all_affected);
            }
        }

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        void _ground() override
        {
            PropertyBodyBase::_ground();
            m_expression = {};
        }

        /// Changes the value of this Property if the new one is different and then updates all affected Properties.
        /// @param value            New value.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        void _set_value(T&& value, Affected& all_affected)
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // no update without change
            if (value == m_value) {
                return;
            }

            // order affected properties upstream before downstream
            if (m_head) {
                all_affected.emplace(this);
            }

            // update the value of yourself and all downstream properties
            m_value = std::forward<T>(value);
            for (valid_ptr<PropertyBody*> affected : m_downstream) {
                affected->_update(all_affected);
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// The head of this Property body (if one exists).
        risky_ptr<PropertyHead*> m_head;

        /// Expression evaluating to a new value for this Property.
        Expression<T> m_expression;

        /// Value held by the Property.
        T m_value;
    };

    //=========================================================================
public:
    class PropertyAccessorBase {

        friend class PropertyBodyBase;

        // methods ------------------------------------------------------------
    public:
        /// Empty default constructor.
        PropertyAccessorBase() = default;

        /// Value constructor.
        /// @param body     Owning pointer to the PropertyBody to access.
        PropertyAccessorBase(PropertyBodyPtr&& body) : m_body(std::move(body)) {}

        /// Copy Constructor.
        /// @param other    Accessor to copy.
        PropertyAccessorBase(const PropertyAccessorBase& other) : m_body(other.m_body) {}

        /// Move Constructor.
        /// @param other    Accessor to move.
        PropertyAccessorBase(PropertyAccessorBase&& other) : m_body(std::move(other.m_body)) { other.m_body.reset(); }

        // fields -------------------------------------------------------------
    protected:
        /// Owning pointer to the PropertyBody to access.
        PropertyBodyPtr m_body;
    };

    template<class T>
    class PropertyAccessor : public PropertyAccessorBase {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param body     Owning pointer to the PropertyBody to access.
        PropertyAccessor(TypedPropertyBodyPtr<T>&& body) : PropertyAccessorBase(std::move(body)) {}

        /// Read-access to the value of the PropertyBody.
        const T& operator()() const { return static_cast<PropertyBody<T>*>(m_body.get())->value(); }
    };

    //=========================================================================

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Mutex guarding all Property bodies.
    /// Needs to be recursive because we need to execute user-defined expressions (that might lock the mutex) while
    /// already holding it.
    static RecursiveMutex s_mutex;
};

using PropertyBodyPtr = PropertyGraph::PropertyBodyPtr;
template<class T>
using TypedPropertyBodyPtr = PropertyGraph::TypedPropertyBodyPtr<T>;

} // namespace temp

NOTF_CLOSE_NAMESPACE
