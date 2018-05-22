#pragma once

#include <atomic>
#include <functional>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _PropertyGraph;
class _PropertyReader_PropertyBody;
template<class>
class _PropertyBodyBase;
template<class>
class _PropertyHeadBase;
} // namespace access

// ================================================================================================================== //

class PropertyReaderBase {

    friend class access::_PropertyReader_PropertyBody;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    using PropertyBodyAccess = access::_PropertyReader_PropertyBody;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Empty default constructor.
    PropertyReaderBase() = default;

    /// Value constructor.
    /// @param body     Owning pointer to the PropertyBody to read from.
    PropertyReaderBase(PropertyBodyBasePtr&& body) : m_body(std::move(body)) {}

    /// Copy Constructor.
    /// @param other    Reader to copy.
    PropertyReaderBase(const PropertyReaderBase& other) : m_body(other.m_body) {}

    /// Move Constructor.
    /// @param other    Reader to move.
    PropertyReaderBase(PropertyReaderBase&& other) : m_body(std::move(other.m_body)) { other.m_body.reset(); }

    /// Equality operator.
    /// @param rhs      Other reader to compare against.
    bool operator==(const PropertyReaderBase& rhs) const { return m_body == rhs.m_body; }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Owning pointer to the PropertyBody to read from.
    PropertyBodyBasePtr m_body;
};

template<class T>
class PropertyReader final : public PropertyReaderBase {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor.
    /// @param body     Owning pointer to the PropertyBody to read from.
    PropertyReader(PropertyBodyPtr<T>&& body) : PropertyReaderBase(std::move(body)) {}

    /// Read-access to the value of the PropertyBody.
    const T& operator()() const { return static_cast<PropertyBody<T>*>(m_body.get())->value(); }
};

// accessors -------------------------------------------------------------------------------------------------------- //

class access::_PropertyReader_PropertyBody {
    friend class notf::PropertyBodyBase;

    /// Mutex guarding all Property bodies.
    static const PropertyBodyBasePtr& property(const PropertyReaderBase& reader) { return reader.m_body; }
};

// ================================================================================================================== //

class PropertyGraph {

    friend class access::_PropertyGraph<PropertyBodyBase>;
    friend class access::_PropertyGraph<PropertyBatch>;
#ifdef NOTF_TEST
    friend class access::_PropertyGraph<test::Harness>;
#endif

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyGraph<T>;

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error);

    /// Checks if T is a valid type to store in a Property.
    template<class T>
    using is_property_type = typename std::conjunction< //
        std::is_copy_constructible<T>,                  // we need to copy property values to store them as deltas
        std::negation<std::is_const<T>>>;               // property values must be modifyable
    template<class T>
    static constexpr bool is_property_type_v = is_property_type<T>::value;

    /// Expression defining a Property of type T.
    template<class T>
    using Expression = std::function<T()>;

    /// Validator function for a Property of type T.
    template<class T>
    using Validator = std::function<bool(T&)>;

    /// Owning references to all property bodies that a Property one depends on through its expression.
    using Dependencies = std::vector<PropertyReaderBase>;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Mutex guarding all Property bodies.
    /// Needs to be recursive because we need to execute user-defined expressions (that might lock the mutex) while
    /// already holding it.
    static RecursiveMutex s_mutex;

#ifdef NOTF_TEST
    /// Number of existing Property bodies (for testing only).
    static std::atomic_size_t s_property_count;
#endif
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_PropertyGraph<PropertyBodyBase> {
    friend class notf::PropertyBodyBase;

#ifdef NOTF_TEST
    /// Direct access to the PropertyGraph's property counter.
    static std::atomic_size_t& property_count() { return PropertyGraph::s_property_count; }
#endif

    /// Mutex guarding all Property bodies.
    static RecursiveMutex& mutex() { return PropertyGraph::s_mutex; }
};

template<>
class access::_PropertyGraph<PropertyBatch> {
    friend class notf::PropertyBatch;

    /// Mutex guarding all Property bodies.
    static RecursiveMutex& mutex() { return PropertyGraph::s_mutex; }
};

//====================================================================================================================//

/// Virtual baseclass, so we can store updates of various property types in one Batch.
struct PropertyUpdate {

protected:
    /// Constructor.
    /// @param target   Property targeted by this update.
    PropertyUpdate(PropertyBodyBasePtr target) : property(std::move(target)) {}

public:
    /// Destructor.
    virtual ~PropertyUpdate();

    /// Property that was updated.
    PropertyBodyBasePtr property;
};

/// List of PropertyUpdates.
using PropertyUpdateList = std::vector<PropertyUpdatePtr>;

/// Stores a single value update for a property.
template<class T>
struct PropertyValueUpdate final : public PropertyUpdate {

    /// Constructor.
    /// @param target   Property targeted by this update.
    /// @param value    New value of the targeted Property.
    PropertyValueUpdate(PropertyBodyBasePtr target, T value)
        : PropertyUpdate(std::move(target)), value(std::forward<T>(value))
    {}

    /// New value of the targeted Property.
    T value; // not const so we can move from it
};

/// Stores an expression update for a property.
template<class T>
struct PropertyExpressionUpdate final : public PropertyUpdate {

    using Expression = PropertyGraph::Expression<T>;
    using Dependencies = PropertyGraph::Dependencies;

    /// Constructor.
    /// @param target           Property targeted by this update.
    /// @param expression       New expression for the targeted Property.
    /// @param dependencies     Property Readers that the expression depends on.
    PropertyExpressionUpdate(PropertyBodyBasePtr target, Expression expression, Dependencies dependencies)
        : PropertyUpdate(std::move(target)), expression(std::move(expression)), dependencies(std::move(dependencies))
    {}

    /// New value of the targeted Property.
    Expression expression; // not const so we can move from it

    /// Properties that the expression depends on.
    Dependencies dependencies; // not const so we can move from it
};

//====================================================================================================================//

class PropertyBodyBase {

    template<class>
    friend class PropertyBody;
    friend class access::_PropertyBodyBase<PropertyBatch>;

    // types ---------------------------------------------------------------------------------------------------------//
private:
    using Dependencies = PropertyGraph::Dependencies;

public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyBodyBase<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
#ifndef NOTF_TEST
    /// Default constructor.
    PropertyBodyBase() = default;
#else
    /// Default constructor.
    PropertyBodyBase() { ++PropertyGraph::Access<PropertyBodyBase>::property_count(); }
#endif
public:
    /// Destructor.
    virtual ~PropertyBodyBase();

    /// The PropertyHead associated with this body, if one exists.
    risky_ptr<PropertyHeadBase*> head() const { return m_head; }

protected:
    /// Updates the Property by evaluating its expression.
    /// Then continues to update all downstream nodes as well.
    /// @param effects      [OUT] All properties affected of the change.
    virtual void _update(PropertyUpdateList& effects) = 0;

    /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
    virtual void _ground();

    /// Checks if a given update would succeed if executed or not.
    /// @param update           Untyped update to test (only PropertyExpressionUpdated can fail).
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    virtual void _validate_update(valid_ptr<PropertyUpdate*> update) = 0;

    /// Allows an untyped Property to ingest an untyped Update from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param update       Update to apply.
    /// @param effects      [OUT] All properties affected of the change.
    virtual void _apply_update(valid_ptr<PropertyUpdate*> update, PropertyUpdateList& effects) = 0;

    /// Tests whether the propposed upstream can be accepted, or would introduce a cyclic dependency.
    /// @param dependencies Dependencies to test.
    /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
    void _test_upstream(const Dependencies& dependencies);

    /// Updates the upstream properties that this one depends on through its expression.
    /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
    void _set_upstream(Dependencies&& dependencies);

    /// Adds a new downstream property that is affected by this one through an expression.
    void _add_downstream(const valid_ptr<PropertyBodyBase*> affected);

    /// Mutex guarding all Property bodies.
    RecursiveMutex& _mutex() const { return PropertyGraph::Access<PropertyBodyBase>::mutex(); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Owning references to all PropertyBodies that this one depends on through its expression.
    Dependencies m_upstream;

    /// PropertyBodies depending on this through their expressions.
    std::vector<valid_ptr<PropertyBodyBase*>> m_downstream;

    /// The head of this Property body (if one exists).
    risky_ptr<PropertyHeadBase*> m_head;
};

template<class T>
class PropertyBody final : public PropertyBodyBase, public std::enable_shared_from_this<PropertyBody<T>> {

    using Expression = PropertyGraph::Expression<T>;
    using std::enable_shared_from_this<PropertyBody<T>>::shared_from_this;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value constructor.
    /// @param value    Value held by the Property, is used to determine the property type.
    PropertyBody(T&& value) : PropertyBodyBase(), m_value(std::forward<T>(value)) {}

public:
    /// Factory function, making sure that all PropertyBodies are managed by a shared_ptr.
    /// @param value    Value held by the Property, is used to determine the property type.
    static PropertyBodyPtr<T> create(T&& value)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PropertyBody<T>, std::forward<T>(value));
    }

    /// Checks if the Property is grounded or not (has an expression).
    bool is_grounded() const
    {
        NOTF_MUTEX_GUARD(_mutex());
        return !static_cast<bool>(m_expression);
    }

    /// Checks if the Property has an expression or not (is grounded).
    bool has_expression() const { return !is_grounded(); }

    /// The Property's value.
    const T& value() const
    {
        NOTF_MUTEX_GUARD(_mutex());
        return m_value;
    }

    /// Sets the Property's value and fires a PropertyEvent.
    /// @param value        New value.
    void set_value(T&& value)
    {
        PropertyUpdateList effects;
        set_value(std::forward<T>(value), effects);
        //            PropertyGraph::PropertyAccess::fire_event(*property_graph, std::move(effects));
        // TODO: fire event
    }

    /// Set the Property's value and updates downstream Properties.
    /// Removes an existing expression on this Property if one exists.
    /// @param value        New value.
    /// @param effects      [OUT] All properties affected of the change.
    void set_value(T&& value, PropertyUpdateList& effects)
    {
        NOTF_MUTEX_GUARD(_mutex());
        if (m_expression) {
            _ground();
        }
        _set_value(std::forward<T>(value), effects);
    }

    /// Set the Property's expression.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(Expression expression, Dependencies dependencies)
    {
        PropertyUpdateList effects;
        set_expression(std::move(expression), std::move(dependencies), effects);
        // TODO: fire event
    }

    /// Set the Property's expression.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @param effects          [OUT] All properties affected of the change.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(Expression expression, Dependencies dependencies, PropertyUpdateList& effects)
    {
        NOTF_MUTEX_GUARD(_mutex());

        // always remove the current expression, even if the new one might be invalid
        _ground();

        if (expression) {
            // update connections on yourself and upstream
            _set_upstream(std::move(dependencies)); // might throw no_dag_error
            m_expression = std::move(expression);

            // update the value of yourself and all downstream properties
            _update(effects);
        }
    }

private:
    /// Changes the value of this Property if the new one is different and then updates all affected Properties.
    /// @param value        New value.
    /// @param effects      [OUT] All properties affected of the change.
    void _set_value(T&& value, PropertyUpdateList& effects)
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // no update without change
        if (value == m_value) {
            return;
        }

        // order affected properties upstream before downstream
        if (m_head) {
            effects.emplace_back(std::make_unique<PropertyValueUpdate<T>>(shared_from_this(), value));
        }

        // update the value of yourself and all downstream properties
        m_value = std::forward<T>(value);
        for (valid_ptr<PropertyBodyBase*> affected : m_downstream) {
            affected->_update(effects);
        }
    }

    /// Sets a Property expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @param effects          [OUT] All properties affected of the change.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    void _set_expression(Expression expression, Dependencies dependencies, PropertyUpdateList& effects)
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // do not accept an empty expression
        if (!expression) {
            return;
        }

        // update connections on yourself and upstream
        _set_upstream(std::move(dependencies)); // might throw no_dag_error
        m_expression = std::move(expression);

        // update the value of yourself and all downstream properties
        _update(effects);
    }

    /// Updates the Property by evaluating its expression.
    /// Then continues to update all downstream nodes as well.
    /// @param effects      [OUT] All properties affected of the change.
    void _update(PropertyUpdateList& effects) override
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        if (m_expression) {
            _set_value(m_expression(), effects);
        }
    }

    /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
    void _ground() override
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        PropertyBodyBase::_ground();
        m_expression = {};
    }

    /// Checks if a given update would succeed if executed or not.
    /// @param update           Untyped update to test (only PropertyExpressionUpdated can fail).
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    void _validate_update(valid_ptr<PropertyUpdate*> update) override
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // check if an exception would introduce a cyclic dependency
        if (PropertyExpressionUpdate<T>* expression_update = dynamic_cast<PropertyExpressionUpdate<T>*>(update.get())) {
            _test_upstream(expression_update->dependencies);
        }
    }

    /// Allows an untyped Property to ingest an untyped Update from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param update       Update to apply.
    /// @param effects      [OUT] All properties affected of the change.
    void _apply_update(valid_ptr<PropertyUpdate*> raw_update, PropertyUpdateList& effects) override
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // update with a ground value
        if (PropertyValueUpdate<T>* update = dynamic_cast<PropertyValueUpdate<T>*>(raw_update.get())) {
            NOTF_ASSERT(update->property.get() == this);
            _set_value(std::forward<T>(update->value), effects);
        }

        // update with an expression
        else if (PropertyExpressionUpdate<T>* update = dynamic_cast<PropertyExpressionUpdate<T>*>(raw_update.get())) {
            NOTF_ASSERT(update->property.get() == this);
            _set_expression(std::move(update->expression), std::move(update->dependencies), effects);
        }

        // should not happen
        else {
            NOTF_ASSERT(false);
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Expression evaluating to a new value for this Property.
    Expression m_expression;

    /// Value held by the Property.
    T m_value;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_PropertyBodyBase<PropertyBatch> {
    friend class notf::PropertyBatch;

    /// Checks if a given update would succeed if executed or not.
    /// @param property         Property to access.
    /// @param update           Untyped update to test (only PropertyExpressionUpdated can fail).
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    static void validate_update(PropertyBodyBasePtr& property, const PropertyUpdatePtr& update)
    {
        property->_validate_update(update.get());
    }

    /// Allows an untyped Property to ingest an untyped Update from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param property         Property to access.
    /// @param update           Update to apply.
    /// @param effects          [OUT] All properties affected of the change.
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    static void
    apply_update(PropertyBodyBasePtr& property, const PropertyUpdatePtr& update, PropertyUpdateList& effects)
    {
        property->_apply_update(update.get(), effects);
    }
};

// ================================================================================================================== //

class PropertyHeadBase {

    friend class access::_PropertyHeadBase<PropertyBatch>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyHeadBase<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @param body     PropertyBoy associated with this head.
    PropertyHeadBase(PropertyBodyBasePtr&& body) : m_body(std::move(body)) {}

public:
    /// Destructor.
    virtual ~PropertyHeadBase();

private:
    virtual void _apply_update(valid_ptr<PropertyUpdate*> update) = 0;

    /// Allows all subclasses of PropertyHeadBase to call `_apply_update` on all others.
    static void _apply_update_to(PropertyHeadBase* property_head, valid_ptr<PropertyUpdate*> update)
    {
        property_head->_apply_update(update);
    }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// The untyped property body.
    PropertyBodyBasePtr m_body;
};

template<class T>
class PropertyHead : public PropertyHeadBase {

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// @param value    Initial value of the Property.
    PropertyHead(T&& value) : PropertyHeadBase(PropertyBody<T>::create(std::forward<T>(value))) {}
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_PropertyHeadBase<PropertyBatch> {
    friend class notf::PropertyBatch;

    /// The untyped property body.
    static PropertyBodyBasePtr& body(PropertyHeadBase& property_head) { return property_head.m_body; }
};

//====================================================================================================================//

class NOTF_NODISCARD PropertyBatch {

    template<class T>
    using Expression = PropertyGraph::Expression<T>;
    using Dependencies = PropertyGraph::Dependencies;

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
    template<class T>
    void set_value(PropertyHead<T>& property, T&& value)
    {
        m_updates.emplace_back(std::make_unique<PropertyValueUpdate<T>>(
            PropertyHeadBase::Access<PropertyBatch>::body(property), std::forward<T>(value)));
    }

    /// Set the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param target           Property targeted by this update.
    /// @param expression       New expression for the targeted Property.
    /// @param dependencies     Property Readers that the expression depends on.
    template<class T>
    void set_expression(PropertyHead<T>& property, identity_t<Expression<T>>&& expression, Dependencies&& dependencies)
    {
        m_updates.emplace_back(std::make_unique<PropertyExpressionUpdate<T>>(
            PropertyHeadBase::Access<PropertyBatch>::body(property), std::move(expression), std::move(dependencies)));
    }

    /// Executes this batch.
    /// If any error occurs, this method will throw the exception and not modify the PropertyGraph.
    /// If no exception occurs, the transaction was successfull and the batch is empty again.
    /// @throws no_dag_error    If a Property expression update would cause a cyclic dependency in the graph.
    void execute();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All updates that make up this batch.
    PropertyUpdateList m_updates;
};

NOTF_CLOSE_NAMESPACE
