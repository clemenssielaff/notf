#pragma once

#include <functional>
#include <set>

#include "app/forwards.hpp"
#include "common/assert.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

class PropertyGraph {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    struct Update;

public:
    template<class, class>
    class PropertyBody;
    class PropertyBodyBase;

    template<class>
    class PropertyReader;
    class PropertyReaderBase;

    class Batch;

    // ------------------------------------------------------------------------
public:
    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error);

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

    //=========================================================================
public: // TODO: make private
    /// Virtual baseclass, so we can store updates of various property types in one Batch.
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
        /// @param dependencies     Property Readers that the expression depends on.
        ExpressionUpdate(PropertyBodyPtr target, Expression<T>&& expression,
                         std::vector<PropertyReaderBase>&& dependencies)
            : Update(std::move(target))
            , expression(std::forward<Expression<T>>(expression))
            , dependencies(std::move(dependencies))
        {}

        /// New value of the targeted Property.
        Expression<T> expression; // not const so we can move from it

        /// Properties that the expression depends on.
        std::vector<PropertyReaderBase> dependencies; // not const so we can move from it
    };

    //=========================================================================
public:
    class NOTF_NODISCARD Batch {

        // methods ------------------------------------------------------------
    public:
        NOTF_NO_COPY_OR_ASSIGN(Batch);

        /// Empty default constructor.
        Batch() = default;

        /// Move Constructor.
        /// @param other    Other Batch to move from.
        Batch(Batch&& other) = default;

        /// Destructor.
        /// Tries to execute but will swallow any exceptions that might occur.
        /// In the case of an exception, the PropertyGraph will not be modified.
        ~Batch()
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
        template<class T>
        void set_value(PropertyPtr<T> property, T&& value)
        {
            m_updates.emplace(std::make_unique<ValueUpdate<T>>(std::move(property), std::forward<T>(value)));
        }

        /// Set the Property's expression.
        /// Evaluates the expression right away to update the Property's value.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Property Readers that the expression depends on.
        template<class T>
        void set_expression(PropertyPtr<T> property, identity_t<Expression<T>>&& expression,
                            std::vector<PropertyReaderBase>&& dependencies)
        {
            m_updates.emplace(std::make_unique<ExpressionUpdate<T>>(
                std::move(property), std::forward<Expression<T>>(expression), std::move(dependencies)));
        }

        /// Executes this batch.
        /// If any error occurs, this method will throw the exception and not modify the PropertyGraph.
        /// If no exception occurs, the transaction was successfull and the batch is empty again.
        /// @throws no_dag_error    If a Property expression update would cause a cyclic dependency in the graph.
        void execute();

        // fields -------------------------------------------------------------
    private:
        /// All updates that make up this batch.
        std::set<std::unique_ptr<Update>> m_updates;
    };

    //=========================================================================
public:
    class PropertyBodyBase {

        template<class, class>
        friend class PropertyBody;

        // types --------------------------------------------------------------
    public: // TODO: make private
        /// Owning references to all PropertyBodies that this one depends on through its expression.
        using Dependencies = std::vector<PropertyReaderBase>;

        /// Set of all property bodies affected by a change in the graph.
        using Affected = std::set<valid_ptr<const PropertyBodyBase*>>;

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        virtual ~PropertyBodyBase();

        // TODO: make private?
        risky_ptr<PropertyHead*> head() const { return m_head; }

        //    protected:
    public: // TODO: make protected
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        virtual void _update(Affected& affected) = 0;

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        virtual void _ground();

        /// Checks if a given update would succeed if executed or not.
        /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
        virtual void _validate_update(valid_ptr<Update*> update) = 0;

        /// Allows an untyped Property to ingest an untyped Update from a Batch.
        /// Note that this method moves the value/expression out of the update.
        /// @param update       Update to apply.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        virtual void _apply_update(valid_ptr<Update*> update, Affected& affected) = 0;

        /// Tests whether the propposed upstream can be accepted, or would introduce a cyclic dependency.
        /// @param dependencies Dependencies to test.
        /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
        void _test_upstream(const Dependencies& dependencies);

        /// Updates the upstream properties that this one depends on through its expression.
        /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
        void _set_upstream(Dependencies&& dependencies);

        /// Adds a new downstream property that is affected by this one through an expression.
        void _add_downstream(const valid_ptr<PropertyBodyBase*> affected);

        // fields -------------------------------------------------------------
    protected:
        /// Owning references to all PropertyBodies that this one depends on through its expression.
        Dependencies m_upstream;

        /// PropertyBodies depending on this through their expressions.
        std::vector<valid_ptr<PropertyBodyBase*>> m_downstream;

        /// The head of this Property body (if one exists).
        risky_ptr<PropertyHead*> m_head;
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
            NOTF_MUTEX_GUARD(PropertyGraph::s_mutex);
            return !static_cast<bool>(m_expression);
        }

        /// Checks if the Property has an expression or not (is grounded).
        bool has_expression() const { return !is_grounded(); }

        /// The Property's value.
        const T& value() const
        {
            NOTF_MUTEX_GUARD(PropertyGraph::s_mutex);
            return m_value;
        }

        /// Sets the Property's value and fires a PropertyEvent.
        /// @param value        New value.
        void set_value(T&& value)
        {
            Affected affected;
            set_value(std::forward<T>(value), affected);
            //            PropertyGraph::PropertyAccess::fire_event(*property_graph, std::move(affected));
            // TODO: fire event
        }

        /// Set the Property's value and updates downstream Properties.
        /// Removes an existing expression on this Property if one exists.
        /// @param value        New value.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        void set_value(T&& value, Affected& affected)
        {
            NOTF_MUTEX_GUARD(PropertyGraph::s_mutex);
            if (m_expression) {
                _ground();
            }
            _set_value(std::forward<T>(value), affected);
        }

        /// Set the Property's expression.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @param affected         [OUT] Set of all associated properties affected of the change.
        /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
        void set_expression(Expression<T> expression, Dependencies dependencies, Affected& affected)
        {
            NOTF_MUTEX_GUARD(PropertyGraph::s_mutex);

            // always remove the current expression, even if the new one might be invalid
            _ground();

            if (expression) {
                // update connections on yourself and upstream
                _set_upstream(std::move(dependencies)); // might throw no_dag_error
                m_expression = std::move(expression);

                // update the value of yourself and all downstream properties
                _update(affected);
            }
        }

    public: // TODO: make private
        /// Changes the value of this Property if the new one is different and then updates all affected Properties.
        /// @param value        New value.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        void _set_value(T&& value, Affected& affected)
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // no update without change
            if (value == m_value) {
                return;
            }

            // order affected properties upstream before downstream
            if (m_head) {
                affected.emplace(this);
            }

            // update the value of yourself and all downstream properties
            m_value = std::forward<T>(value);
            for (valid_ptr<PropertyBody*> affected : m_downstream) {
                affected->_update(affected);
            }
        }

        /// Sets a Property expression.
        /// Evaluates the expression right away to update the Property's value.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @param affected         [OUT] Set of all associated properties affected of the change.
        /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
        void _set_expression(Expression<T> expression, Dependencies dependencies, Affected& affected)
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // do not accept an empty expression
            if (!expression) {
                return;
            }

            // update connections on yourself and upstream
            _set_upstream(std::move(dependencies)); // might throw no_dag_error
            m_expression = std::move(expression);

            // update the value of yourself and all downstream properties
            _update(affected);
        }

        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        void _update(Affected& affected) override
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            if (m_expression) {
                _set_value(m_expression(), affected);
            }
        }

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        void _ground() override
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            PropertyBodyBase::_ground();
            m_expression = {};
        }

        /// Checks if a given update would succeed if executed or not.
        /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
        void _validate_update(valid_ptr<Update*> update) override
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // check if an exception would introduce a cyclic dependency
            if (ExpressionUpdate<T>* expression_update = dynamic_cast<ExpressionUpdate<T>*>(update.get())) {
                _test_upstream(expression_update->dependencies);
            }
        }

        /// Allows an untyped Property to ingest an untyped Update from a Batch.
        /// Note that this method moves the value/expression out of the update.
        /// @param update       Update to apply.
        /// @param affected     [OUT] Set of all associated properties affected of the change.
        void _apply_update(valid_ptr<Update*> raw_update, Affected& affected) override
        {
            NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

            // update with a ground value
            if (ValueUpdate<T>* update = dynamic_cast<ValueUpdate<T>*>(raw_update.get())) {
                NOTF_ASSERT(update->property.get() == this);
                _set_value(std::forward<T>(update->value), affected);
            }

            // update with an expression
            else if (ExpressionUpdate<T>* update = dynamic_cast<ExpressionUpdate<T>*>(raw_update.get())) {
                NOTF_ASSERT(update->property.get() == this);
                _set_expression(std::move(update->expression), std::move(update->dependencies), affected);
            }

            // should not happen
            else {
                NOTF_ASSERT(false);
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Expression evaluating to a new value for this Property.
        Expression<T> m_expression;

        /// Value held by the Property.
        T m_value;
    };

    //=========================================================================
public:
    class PropertyReaderBase {

        friend class PropertyBodyBase;

        // methods ------------------------------------------------------------
    public:
        /// Empty default constructor.
        PropertyReaderBase() = default;

        /// Value constructor.
        /// @param body     Owning pointer to the PropertyBody to read from.
        PropertyReaderBase(PropertyBodyPtr&& body) : m_body(std::move(body)) {}

        /// Copy Constructor.
        /// @param other    Reader to copy.
        PropertyReaderBase(const PropertyReaderBase& other) : m_body(other.m_body) {}

        /// Move Constructor.
        /// @param other    Reader to move.
        PropertyReaderBase(PropertyReaderBase&& other) : m_body(std::move(other.m_body)) { other.m_body.reset(); }

        bool operator==(const PropertyReaderBase& rhs) const { return m_body == rhs.m_body; }

        // fields -------------------------------------------------------------
    protected:
        /// Owning pointer to the PropertyBody to read from.
        PropertyBodyPtr m_body;
    };

    template<class T>
    class PropertyReader : public PropertyReaderBase {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param body     Owning pointer to the PropertyBody to read from.
        PropertyReader(TypedPropertyBodyPtr<T>&& body) : PropertyReaderBase(std::move(body)) {}

        /// Read-access to the value of the PropertyBody.
        const T& operator()() const { return static_cast<PropertyBody<T>*>(m_body.get())->value(); }
    };

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

//====================================================================================================================//

class PropertyHead {

public:
    virtual ~PropertyHead();

public: // TODO: make private
    virtual void _apply_update(valid_ptr<PropertyGraph::Update*> update) = 0;
};

NOTF_CLOSE_NAMESPACE
