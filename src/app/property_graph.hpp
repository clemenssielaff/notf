#pragma once

#include <vector>

#ifdef NOTF_TEST
#include <atomic>
#endif

#include "app/property_reader.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _PropertyGraph;
template<class>
class _PropertyBody;
template<class>
class _PropertyHead;
} // namespace access

// ================================================================================================================== //

class PropertyGraph {

    friend class access::_PropertyGraph<PropertyBody>;
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
    using Dependencies = std::vector<PropertyReader>;

    /// List of PropertyUpdates.
    using PropertyUpdateList = std::vector<PropertyUpdatePtr>;

    // methods ------------------------------------------------------------------------------------------------------ //

    /// Generates one or more PropertyEvents targeted at the SceneGraphs of affected SceneProperties.
    /// @throws Application::shut_down_error    If the global Application has already finished.
    static void fire_event(PropertyUpdateList&& effects);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Mutex guarding all Property bodies.
    /// Needs to be recursive because we need to execute user-defined expressions (that might lock the mutex) while
    /// already holding it.
    static RecursiveMutex s_mutex;

#ifdef NOTF_TEST
    /// Number of existing Property bodies (for testing only).
    static std::atomic_size_t s_body_count;
#endif
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_PropertyGraph<PropertyBody> {
    friend class notf::PropertyBody;

#ifdef NOTF_TEST
    /// Direct access to the PropertyGraph's property counter.
    static std::atomic_size_t& property_count() { return PropertyGraph::s_body_count; }
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

// ================================================================================================================== //

/// Virtual baseclass, so we can store updates of various property types in one PropertyBatch or -Event.
struct PropertyUpdate {

protected:
    /// Constructor.
    /// @param target   Property targeted by this update.
    PropertyUpdate(PropertyBodyPtr target) : property(std::move(target)) {}

public:
    /// Destructor.
    virtual ~PropertyUpdate();

    /// Property that was updated.
    PropertyBodyPtr property;
};

/// Stores a single value update for a property.
template<class T>
struct PropertyValueUpdate final : public PropertyUpdate {

    /// Constructor.
    /// @param target   Property targeted by this update.
    /// @param value    New value of the targeted Property.
    PropertyValueUpdate(PropertyBodyPtr target, T value)
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
    PropertyExpressionUpdate(PropertyBodyPtr target, Expression expression, Dependencies dependencies)
        : PropertyUpdate(std::move(target)), expression(std::move(expression)), dependencies(std::move(dependencies))
    {}

    /// New value of the targeted Property.
    Expression expression; // not const so we can move from it

    /// Properties that the expression depends on.
    Dependencies dependencies; // not const so we can move from it
};

// ================================================================================================================== //

class PropertyBody {

    template<class>
    friend class TypedPropertyBody;
    friend class access::_PropertyBody<PropertyBatch>;
    friend class access::_PropertyBody<PropertyHead>;

    // types -------------------------------------------------------------------------------------------------------- //
private:
    using Dependencies = PropertyGraph::Dependencies;
    using PropertyUpdateList = PropertyGraph::PropertyUpdateList;

public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyBody<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Value constructor.
    /// @param head     Head of this Property body.
    PropertyBody(valid_ptr<PropertyHead*> head) : m_head(std::move(head))
    {
#ifdef NOTF_TEST
        ++PropertyGraph::Access<PropertyBody>::property_count();
#endif
    }

public:
    /// Destructor.
    virtual ~PropertyBody();

    /// The PropertyHead associated with this body, if one exists.
    risky_ptr<PropertyHead*> head() const { return m_head; }

protected:
    /// Updates the Property by evaluating its expression.
    /// Then continues to update all downstream nodes as well.
    /// @param effects      [OUT] All properties affected by the change.
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
    /// @param effects      [OUT] All properties affected by the change.
    virtual void _apply_update(valid_ptr<PropertyUpdate*> update, PropertyUpdateList& effects) = 0;

    /// Tests whether the propposed upstream can be accepted, or would introduce a cyclic dependency.
    /// @param dependencies Dependencies to test.
    /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
    void _test_upstream(const Dependencies& dependencies);

    /// Updates the upstream properties that this one depends on through its expression.
    /// @throws no_dag_error    If the connections would introduce a cyclic dependency into the graph.
    void _set_upstream(Dependencies&& dependencies);

    /// Adds a new downstream property that is affected by this one through an expression.
    void _add_downstream(valid_ptr<PropertyBody*> affected);

    /// Called by the head of this Property when it is deleted.
    void _remove_head();

    /// Mutex guarding all Property bodies.
    RecursiveMutex& _mutex() const { return PropertyGraph::Access<PropertyBody>::mutex(); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Owning references to all PropertyBodies that this one depends on through its expression.
    Dependencies m_upstream;

    /// PropertyBodies depending on this through their expressions.
    std::vector<valid_ptr<PropertyBody*>> m_downstream;

    /// The head of this Property body (if one exists).
    risky_ptr<PropertyHead*> m_head;
};

template<class T>
class TypedPropertyBody : public PropertyBody, public std::enable_shared_from_this<TypedPropertyBody<T>> {

    using Expression = PropertyGraph::Expression<T>;
    using std::enable_shared_from_this<TypedPropertyBody<T>>::shared_from_this;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value constructor.
    /// @param value    Value held by the Property, is used to determine the property type.
    TypedPropertyBody(valid_ptr<PropertyHead*> head, T&& value) : PropertyBody(head), m_value(std::forward<T>(value)) {}

public:
    /// Factory function, making sure that all PropertyBodies are managed by a shared_ptr.
    /// @param value    Value held by the Property, is used to determine the property type.
    static std::enable_if_t<PropertyGraph::is_property_type_v<T>, TypedPropertyBodyPtr<T>>
    create(valid_ptr<PropertyHead*> head, T&& value)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(TypedPropertyBody<T>, head, std::forward<T>(value));
    }

    /// Checks if the Property is grounded or not (has an expression).
    bool is_grounded() const
    {
        NOTF_GUARD(std::lock_guard(_mutex()));
        return !static_cast<bool>(m_expression);
    }

    /// Checks if the Property has an expression or not (is grounded).
    bool has_expression() const { return !is_grounded(); }

    /// The Property's value.
    const T& get() const
    {
        NOTF_GUARD(std::lock_guard(_mutex()));
        return m_value;
    }

    /// Sets the Property's value and fires a PropertyEvent.
    /// Fires a PropertyEvent, if the call had any effect.
    /// @param value        New value.
    void set(T&& value)
    {
        PropertyUpdateList effects;
        set(std::forward<T>(value), effects);
        PropertyGraph::fire_event(std::move(effects));
    }

    /// Set the Property's expression.
    /// Fires a PropertyEvent, if the call had any effect.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set(Expression expression, Dependencies dependencies)
    {
        PropertyUpdateList effects;
        set(std::move(expression), std::move(dependencies), effects);
        PropertyGraph::fire_event(std::move(effects));
    }

    /// Set the Property's value and updates downstream Properties.
    /// Removes an existing expression on this Property if one exists.
    /// Does not fire a PropertyEvent, but instead passes all effects into the output argument.
    /// @param value        New value.
    /// @param effects      [OUT] All properties affected by the change.
    void set(T&& value, PropertyUpdateList& effects)
    {
        NOTF_GUARD(std::lock_guard(_mutex()));
        _ground();
        _set_value(std::forward<T>(value), effects);
    }

    /// Set the Property's expression.
    /// Does not fire a PropertyEvent, but instead passes all effects into the output argument.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @param effects          [OUT] All properties affected by the change.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set(Expression expression, Dependencies dependencies, PropertyUpdateList& effects)
    {
        NOTF_GUARD(std::lock_guard(_mutex()));
        _ground(); // always remove the current expression, even if the new one might be invalid
        _set_expression(std::move(expression), std::move(dependencies), effects);
    }

private:
    /// Changes the value of this Property if the new one is different and then updates all affected Properties.
    /// @param value        New value.
    /// @param effects      [OUT] All properties affected by the change.
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
        for (valid_ptr<PropertyBody*> affected : m_downstream) {
            affected->_update(effects);
        }
    }

    /// Sets a Property expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @param effects          [OUT] All properties affected by the change.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
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
    /// @param effects      [OUT] All properties affected by the change.
    void _update(PropertyUpdateList& effects) final
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        if (m_expression) {
            _set_value(m_expression(), effects);
        }
    }

    /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
    void _ground() final
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        if (m_expression) {
            PropertyBody::_ground();
            m_expression = {};
        }
    }

    /// Checks if a given update would succeed if executed or not.
    /// @param update           Untyped update to test (only PropertyExpressionUpdated can fail).
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    void _validate_update(valid_ptr<PropertyUpdate*> update) final
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // check if an exception would introduce a cyclic dependency
        if (auto* expression_update = dynamic_cast<PropertyExpressionUpdate<T>*>(update.get())) {
            _test_upstream(expression_update->dependencies);
        }
    }

    /// Allows an untyped Property to ingest an untyped Update from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param update       Update to apply.
    /// @param effects      [OUT] All properties affected by the change.
    void _apply_update(valid_ptr<PropertyUpdate*> raw_update, PropertyUpdateList& effects) final
    {
        NOTF_ASSERT(_mutex().is_locked_by_this_thread());

        // update with a ground value
        if (auto* update = dynamic_cast<PropertyValueUpdate<T>*>(raw_update.get())) {
            NOTF_ASSERT(update->property.get() == this);
            _set_value(std::forward<T>(update->value), effects);
        }

        // update with an expression
        else if (auto* update = dynamic_cast<PropertyExpressionUpdate<T>*>(raw_update.get())) {
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
class access::_PropertyBody<PropertyBatch> {
    friend class notf::PropertyBatch;

    using PropertyUpdateList = PropertyGraph::PropertyUpdateList;

    /// Checks if a given update would succeed if executed or not.
    /// @param property         Property to access.
    /// @param update           Untyped update to test (only PropertyExpressionUpdated can fail).
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    static void validate_update(PropertyBodyPtr& property, const PropertyUpdatePtr& update)
    {
        property->_validate_update(update.get());
    }

    /// Allows an untyped Property to ingest an untyped Update from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param property         Property to access.
    /// @param update           Update to apply.
    /// @param effects          [OUT] All properties affected by the change.
    /// @throws no_dag_error    If the update is an expression that would introduce a cyclic dependency.
    static void apply_update(PropertyBodyPtr& property, const PropertyUpdatePtr& update, PropertyUpdateList& effects)
    {
        property->_apply_update(update.get(), effects);
    }
};

template<>
class access::_PropertyBody<PropertyHead> {
    friend class notf::PropertyHead;

    /// Called by the head of this Property when it is deleted.
    static void remove_head(PropertyBody& property) { property._remove_head(); }
};

// ================================================================================================================== //

class PropertyHead {

    friend class access::_PropertyHead<PropertyBatch>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyHead<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Empty default Constructor.
    PropertyHead() = default;

    /// Value Constructor.
    /// @param body     PropertyBoy associated with this head.
    template<class T>
    explicit PropertyHead(T value) : m_body(TypedPropertyBody<T>::create(this, std::forward<T>(value)))
    {}

public:
    /// Destructor.
    virtual ~PropertyHead();

    /// Returns the Node associated with this PropertyHead (if there is one).
    virtual risky_ptr<Node*> node() { return nullptr; }

protected:
    /// Updates the value in response to a PropertyEvent.
    /// @param update   PropertyUpdate to apply.
    virtual void _apply_update(valid_ptr<PropertyUpdate*> update) = 0;

    /// Allows all subclasses of PropertyHead to call `_apply_update` on all others.
    static void _apply_update_to(PropertyHead* property_head, valid_ptr<PropertyUpdate*> update)
    {
        property_head->_apply_update(update);
    }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// The untyped property body.
    PropertyBodyPtr m_body;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_PropertyHead<PropertyBatch> {
    friend class notf::PropertyBatch;

    /// The untyped property body.
    static PropertyBodyPtr& body(PropertyHead& property_head) { return property_head.m_body; }
};

NOTF_CLOSE_NAMESPACE
