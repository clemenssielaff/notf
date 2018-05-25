#pragma once

#include <map>

#include "app/property_graph.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _SceneProperty;
} // namespace access

//====================================================================================================================//

class SceneProperty : public PropertyHead {

    friend class access::_SceneProperty<SceneNode>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class T>
    using Access = access::_SceneProperty<T>;

    /// Map to store Properties by their name.
    using PropertyMap = std::map<std::string, std::unique_ptr<SceneProperty>>;

    /// Exception thrown when the initial value of a SceneProperty could not be validated.
    NOTF_EXCEPTION_TYPE(initial_value_error);

    /// Exception thrown when a PropertyHead without a body tries to access one.
    NOTF_EXCEPTION_TYPE(no_body_error);

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// Does not create a PropertyBody for this -head.
    SceneProperty(valid_ptr<SceneNode*> node) : PropertyHead(), m_node(node) {}

    /// Constructor.
    /// Creates an associated PropertyBody for this -head.
    template<class T>
    SceneProperty(T value, valid_ptr<SceneNode*> node) : PropertyHead(std::forward<T>(value)), m_node(node)
    {}

public:
    /// Destructor.
    ~SceneProperty() override;

protected:
    /// Tests if the given SceneNode is currently frozen.
    /// Is implemented in the .cpp file to avoid circular includes.
    bool _is_frozen() const;

    /// Tests if the given SceneNode is currently frozen by a specific thread.
    /// Is implemented in the .cpp file to avoid circular includes.
    /// @param thread_id    Id of the thread to test for.
    bool _is_frozen_by(const std::thread::id& thread_id) const;

    /// The name of the SceneNode.
    const std::string& _node_name() const;

    /// Registers a SceneNode as being "tweaked".
    /// A SceneNode is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    void _set_node_tweaked() const;

    /// Registers a SceneNode as being "dirty".
    /// A SceneNode is dirty when it requires a redraw.
    void _set_node_dirty() const;

private:
    /// Deletes the frozen value copy of the SceneProperty if one exists.
    virtual void _clear_frozen_value() = 0;

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// SceneNode owning this SceneProperty
    valid_ptr<SceneNode*> m_node;

    /// Iterator to this Property in the SceneNode's PropertyMap. Used to access this Property's name.
    PropertyMap::const_iterator m_name_it = {};
};

//====================================================================================================================//

template<class T>
class TypedSceneProperty final : public SceneProperty {

    friend class access::_SceneProperty<SceneNode>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
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
    TypedSceneProperty(T value, valid_ptr<SceneNode*> node, Validator validator)
        : SceneProperty(node), m_validator(std::move(validator)), m_value(std::move(value))
    {}

    /// Constructor.
    /// Creates an associated PropertyBody for this -head.
    TypedSceneProperty(T value, valid_ptr<SceneNode*> node, Validator validator, CreateBody)
        : SceneProperty(value, node), m_validator(std::move(validator)), m_value(std::move(value))
    {}

    /// Factory.
    /// @param value        Initial value of the Property.
    /// @param node         SceneNode owning this SceneProperty
    /// @param validator    Validator function of this Property.
    /// @param create_body  Iff true, the SceneProperty will have an assoicated PropertyBody available in the -graph.
    static TypedScenePropertyPtr<T>
    create(T&& value, valid_ptr<SceneNode*> node, Validator validator, const bool create_body)
    {
        if (create_body) {
            return NOTF_MAKE_UNIQUE_FROM_PRIVATE(TypedSceneProperty<T>, std::forward<T>(value), node,
                                                 std::move(validator), CreateBody{});
        }
        else {
            return NOTF_MAKE_UNIQUE_FROM_PRIVATE(TypedSceneProperty<T>, std::forward<T>(value), node,
                                                 std::move(validator));
        }
    }

public:
    /// Destructor.
    ~TypedSceneProperty() { _clear_frozen_value(); }

    /// Returns the SceneNode associated with this PropertyHead.
    virtual risky_ptr<SceneNode*> scene_node() override { return m_node; }

    /// The node-unique name of this Property.
    const std::string& name() const { return m_name_it->first; }

    /// Current SceneProperty value.
    const T& value() const
    {
        // if the property is frozen by this thread (the render thread, presumably) and there exists a frozen copy of
        // the value, use that instead of the current one
        if (_is_frozen_by(std::this_thread::get_id())) {
            if (T* frozen_value = m_frozen_value.load(std::memory_order_consume)) {
                return *frozen_value;
            }
        }
        return m_value;
    }

    /// Returns true if this SceneProperty can be set to hold an expressions.
    /// If this method returns false, `set_expression` will throw a `no_body_error`.
    bool supports_expressions() const { return (_body() != nullptr); }

    /// Whether or not changing this property will make the SceneNode dirty (cause a redraw) or not.
    bool is_external() const { return m_is_external; }

    /// Set the Property's value.
    /// Removes an existing expression on this Property if one exists.
    /// @param value    New value.
    /// @returns        True iff the value could be updated, false if the validation failed.
    bool set_value(T value)
    {
        // do nothing if the value fails to validate
        if (m_validator && !m_validator(value)) {
            return false;
        }

        if (risky_ptr<TypedPropertyBody<T>*> body = _body()) {
            PropertyUpdateList effects;
            body->set_value(std::forward<T>(value), effects);
            _update_affected(std::move(effects));
        }
        else {
            _set_value(std::move(value));
        }
        return true;
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
        risky_ptr<TypedPropertyBody<T>*> body = _body();
        if (!body) {
            notf_throw_format(TypedSceneProperty<T>::no_body_error,
                              "Property \"{}\" on Node \"{}\" cannot be defined using an Expression", name(),
                              _node_name());
        }

        PropertyUpdateList effects;
        body->_set_expression(std::move(expression), std::move(dependencies), effects);
        _update_affected(std::move(effects));
    }

    /// External properties cause the SceneNode to redraw when changed.
    void set_external(const bool is_external = true) { m_is_external = is_external; }
    void set_internal(const bool is_internal = true) { set_external(!is_internal); }

private:
    /// The typed property body.
    risky_ptr<TypedPropertyBody<T>*> _body() const { return static_cast<TypedPropertyBody<T>*>(m_body.get()); }

    /// Shallow update of affected PropertyHandlers
    /// @param effects  Effects on other Properties that were affected by a change to this one.
    void _update_affected(PropertyUpdateList&& effects)
    {
        for (const PropertyUpdatePtr& update : effects) {
            if (risky_ptr<PropertyHead*> affected_head = update->property->head()) {
                _apply_update_to(affected_head, update.get());
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
        typed_update = dynamic_cast<PropertyValueUpdate<T>*>(update.get());
        NOTF_ASSERT(typed_update);
#else
        typed_update = static_cast<PropertyValueUpdate<T>*>(update);
#endif
        _set_value(std::move(typed_update->value));
    }

    /// Updates the value of the SceneProperty.
    /// @param value    New value.
    void _set_value(T&& value)
    {
        // do nothing if the property value would not actually change
        if (value == m_value) {
            return;
        }

        // do nothing if the value fails to validate
        if (m_validator && !m_validator(value)) {
            return;
        }

        // if the property is currently frozen and this is the first modification, create a frozen copy of the current
        // value before changing it
        if (_is_frozen()) {
            T* frozen_value = m_frozen_value.load(std::memory_order_relaxed);
            if (!frozen_value) {
                frozen_value = new T(std::move(m_value));
                m_frozen_value.store(frozen_value, std::memory_order_release);
                _set_node_tweaked();
            }
        }

        // if the property is external, changing it dirties the node
        if (m_is_external) {
            _set_node_dirty();
        }

        m_value = value;
        on_value_changed(m_value);
    }

    /// Deletes the frozen value copy.
    void _clear_frozen_value() override
    {
        if (T* frozen_ptr = m_frozen_value.exchange(nullptr)) {
            delete frozen_ptr;
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Optional validator function used to validate a given value.
    Validator m_validator;

    /// Pointer to a frozen copy of the value, if it was modified while the SceneGraph was frozen.
    std::atomic<T*> m_frozen_value{nullptr};

    /// Whether or not changing this property will make the SceneNode dirty (cause a redraw) or not.
    bool m_is_external = true;

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
    static TypedScenePropertyPtr<T> create(T value, valid_ptr<SceneNode*> node,
                                           typename TypedSceneProperty<T>::Validator validator, const bool create_body)
    {
        return TypedSceneProperty<T>::create(std::forward<T>(value), node, std::move(validator), create_body);
    }

    /// Deletes the frozen value copy of the SceneProperty if one exists.
    static void clear_frozen(SceneProperty& property) { property._clear_frozen_value(); }

    /// Updates the name of a SceneProperty, which is stored as an iterator to the SceneNode's property map.
    static void set_name_iterator(SceneProperty& property, decltype(SceneProperty::m_name_it) iterator)
    {
        property.m_name_it = iterator;
    }
};

NOTF_CLOSE_NAMESPACE
