#pragma once

#include "app/property_graph.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

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

    using PropertyHead<T>::m_body;
    using PropertyHead<T>::_apply_update_to;
    using Dependencies = PropertyGraph::Dependencies;
    using PropertyUpdateList = PropertyGraph::PropertyUpdateList;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using Expression = PropertyGraph::Expression<T>;
    using Validator = PropertyGraph::Validator<T>;

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Fired when the value of the Property Handler changed.
    Signal<const T&> on_value_changed;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Constructor.
    /// @param value    Initial value of the Property.
    /// @param node     SceneNode owning this SceneProperty
    SceneProperty(T value, valid_ptr<SceneNode*> node) : m_node(node), m_value(std::move(value)) {}

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
    /// @throws no_graph_error  If the PropertyGraph has been deleted.
    void set_value(T&& value)
    {
        PropertyUpdateList effects;
        _body()->set_value(std::forward<T>(value), effects);
        _update_affected(std::move(effects));
    }

    /// Sets the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph_error  If the PropertyGraph has been deleted.
    void _set_expression(Expression&& expression, Dependencies&& dependencies)
    {
        PropertyUpdateList effects;
        _body()->_set_expression(std::move(expression), std::move(dependencies), effects);
        _update_affected(std::move(effects));
    }

private:
    /// The typed property body.
    PropertyBody<T>& _body() const { return *(static_cast<PropertyBody<T>*>(m_body.get())); }

    /// Shallow update of affected PropertyHandlers
    /// @param effects  Effects on other Properties that were affected by a change to this one.
    void _update_affected(PropertyUpdateList&& effects)
    {
        for (const PropertyUpdatePtr& update : effects) {
            if (risky_ptr<PropertyHeadBase*> affected_head = update->property->head()) {
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
        typed_update = dynamic_cast<PropertyValueUpdate<T>*>(update);
        NOTF_ASSERT(typed_update);
#else
        typed_update = static_cast<PropertyValueUpdate<T>*>(update);
#endif
        // early out
        if (typed_update->value == m_value) {
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

    /// Pointer to a frozen copy of the value, if it was modified while the SceneGraph was frozen.
    std::atomic<T*> m_frozen_value;

    /// Current SceneProperty value.
    T m_value;
};

NOTF_CLOSE_NAMESPACE
