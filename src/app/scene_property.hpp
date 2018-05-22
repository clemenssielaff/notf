#pragma once

#include "app/property_graph.hpp"
#include "app/scene_node.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _SceneProperty;
} // namespace access

//====================================================================================================================//

namespace detail {

/// Tests if the given SceneNode is currently frozen.
/// Is implemented in the .cpp file to avoid circular includes.
/// @param node         SceneNode to test.
inline bool is_node_frozen(valid_ptr<SceneNode*> node);

/// Tests if the given SceneNode is currently frozen by a specific thread.
/// Is implemented in the .cpp file to avoid circular includes.
/// @param node         SceneNode to test.
/// @param thread_id    Id of the thread to test for.
inline bool is_node_frozen_by(valid_ptr<SceneNode*> node, const std::thread::id& thread_id);

} // namespace detail

//====================================================================================================================//

template<class T>
class SceneProperty final : public PropertyHead<T> {

    friend class access::_SceneProperty<SceneNode>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class U>
    using Access = access::_SceneProperty<U>;

    /// Exception thrown when the initial value of a SceneProperty could not be validated.
    using PropertyHeadBase::initial_value_error;

    /// Exception thrown when a PropertyHead without a body tries to access one.
    using PropertyHeadBase::no_body_error;

    /// Expression defining a Property of type T.
    using Expression = PropertyGraph::Expression<T>;

    /// Validator function for a Property of type T.
    using Validator = PropertyGraph::Validator<T>;

private:
    using Dependencies = PropertyGraph::Dependencies;
    using PropertyUpdateList = PropertyGraph::PropertyUpdateList;

    /// Helper type to facilitate constructor overloading.
    struct CreateBody {};

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Fired when the value of the Property Handler changed.
    Signal<const T&> on_value_changed;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// Does not create a PropertyBody for this -head.
    SceneProperty(T value, valid_ptr<SceneNode*> node, Validator validator)
        : PropertyHead<T>(), m_node(node), m_validator(std::move(validator)), m_value(std::move(value))
    {}

    /// Constructor.
    /// Creates an associated PropertyBody for this -head.
    SceneProperty(T value, valid_ptr<SceneNode*> node, Validator validator, CreateBody)
        : PropertyHead<T>(value), m_node(node), m_validator(std::move(validator)), m_value(std::move(value))
    {}

    /// Factory.
    /// @param value        Initial value of the Property.
    /// @param node         SceneNode owning this SceneProperty
    /// @param validator    Validator function of this Property.
    /// @param create_body  Iff true, the SceneProperty will have an assoicated PropertyBody available in the -graph.
    static ScenePropertyPtr<T>
    create(T&& value, valid_ptr<SceneNode*> node, Validator validator, const bool create_body)
    {
        if (create_body) {
            return NOTF_MAKE_UNIQUE_FROM_PRIVATE(SceneProperty<T>, std::forward<T>(value), node, std::move(validator),
                                                 CreateBody{});
        }
        else {
            return NOTF_MAKE_UNIQUE_FROM_PRIVATE(SceneProperty<T>, std::forward<T>(value), node, std::move(validator));
        }
    }

public:
    /// Destructor.
    ~SceneProperty()
    {
        // delete the frozen value, if there is one
        if (T* frozen_ptr = m_frozen_value.exchange(nullptr)) {
            delete frozen_ptr;
        }
    }
    /// Returns the SceneNode associated with this PropertyHead.
    virtual risky_ptr<SceneNode*> scene_node() override { return m_node; }

    /// Current SceneProperty value.
    const T& value() const
    {
        // if the property is frozen by this thread (the render thread, presumably) and there exists a frozen copy of
        // the value, use that instead of the current one
        if (detail::is_node_frozen_by(m_node, std::this_thread::get_id())) {
            if (T* frozen_value = m_frozen_value.load(std::memory_order_consume)) {
                return *frozen_value;
            }
        }
        return m_value;
    }

    /// Set the Property's value.
    /// Removes an existing expression on this Property if one exists.
    /// @param value            New value.
    void set_value(T&& value)
    {
        // do nothing if the value fails to validate
        if (m_validator && !m_validator(value)) {
            return;
        }

        m_value = value;

        if (risky_ptr<PropertyBody<T>*> body = _body()) {
            PropertyUpdateList effects;
            body()->set_value(std::forward<T>(value), effects);
            _update_affected(std::move(effects));
        }

        on_value_changed(m_value);
    }

    /// Sets the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param expression   Expression to set.
    /// @param dependencies Properties that the expression depends on.
    /// @throws no_dag_error
    ///                     If the expression would introduce a cyclic dependency into the graph.
    /// @throws SceneProperty<T>::no_body_error
    ///                     If this SceneProperty was created without a PropertyBody and cannot accept expressions.
    void set_expression(Expression&& expression, Dependencies&& dependencies)
    {
        risky_ptr<PropertyBody<T>*> body = _body();
        if (!body) {
            notf_throw(SceneProperty<T>::no_body_error, "Property cannot be defined using an Expression");
            // TODO: should Properties know their own name using an iterator to a map in the SceneNode?
            // TODO: should bodyless Properties be their own type so we don't even have the `set_expression` method when it is not allowed?
        }

        PropertyUpdateList effects;
        body->_set_expression(std::move(expression), std::move(dependencies), effects);

        // do nothing if the value fails to validate
        T new_value = body->value();
        if (m_validator && !m_validator(new_value)) {
            return;
        }
        m_value = std::move(new_value);

        _update_affected(std::move(effects));
        on_value_changed(m_value);
    }

private:
    /// The typed property body.
    risky_ptr<PropertyBody<T>*> _body() const { return static_cast<PropertyBody<T>*>(PropertyHead<T>::m_body.get()); }

    /// Shallow update of affected PropertyHandlers
    /// @param effects  Effects on other Properties that were affected by a change to this one.
    void _update_affected(PropertyUpdateList&& effects)
    {
        for (const PropertyUpdatePtr& update : effects) {
            if (risky_ptr<PropertyHeadBase*> affected_head = update->property->head()) {
                PropertyHead<T>::_apply_update_to(affected_head, update.get());
            }
        }
    }

    /// Updates the value in response to a PropertyEvent.
    /// @param update   PropertyUpdate to apply.
    void _apply_update(valid_ptr<PropertyUpdate*> update) override
    {
        // restore the correct update type
        PropertyValueUpdate<T>* typed_update;
#ifdef NOTF_DEBUG
        typed_update = dynamic_cast<PropertyValueUpdate<T>*>(update);
        NOTF_ASSERT(typed_update);
#else
        typed_update = static_cast<PropertyValueUpdate<T>*>(update);
#endif
        // do nothing if the property value would not actually change
        if (typed_update->value == m_value) {
            return;
        }

        // do nothing if the value fails to validate
        if (m_validator && !m_validator(typed_update->value)) {
            return;
        }

        // if the property is currently frozen and this is the first modification, create a frozen copy of the current
        // value before changing it
        if (detail::is_node_frozen(m_node)) {
            T* frozen_value = m_frozen_value.load(std::memory_order_relaxed);
            if (!frozen_value) {
                frozen_value = new T(std::move(m_value));
                m_frozen_value.store(frozen_value, std::memory_order_release);
            }
        }
        m_value = typed_update->value;
        on_value_changed(m_value);
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// SceneNode owning this SceneProperty
    valid_ptr<SceneNode*> m_node;

    /// Optional validator function used to validate a given value.
    Validator m_validator;

    /// Pointer to a frozen copy of the value, if it was modified while the SceneGraph was frozen.
    std::atomic<T*> m_frozen_value = nullptr;

    /// Current SceneProperty value.
    T m_value;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_SceneProperty<SceneNode> {
    friend class notf::SceneNode;

    /// Factory.
    /// @param value        Initial value of the Property.
    /// @param node         SceneNode owning this SceneProperty
    /// @param validator    Validator function of this Property.
    /// @param create_body  Iff true, the SceneProperty will have an assoicated PropertyBody available in the -graph.
    template<class T>
    static ScenePropertyPtr<T>
    create(T value, valid_ptr<SceneNode*> node, typename SceneProperty<T>::Validator validator, const bool create_body)
    {
        return SceneProperty<T>::create(std::forward<T>(value), node, std::move(validator), create_body);
    }
};

NOTF_CLOSE_NAMESPACE
