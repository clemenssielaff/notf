#pragma once

#include <atomic>
#include <set>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/hash.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

class PropertyGraph : public std::enable_shared_from_this<PropertyGraph> {

    template<typename T>
    friend class Property;

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// Expression defining a Property of type T.
    template<typename T>
    using Expression = std::function<T()>;

    // forwards
    class PropertyBody;
    class PropertyHead;
    template<typename, typename>
    class TypedPropertyBody;

    /// Convenience typedef for owning pointer to property heads.
    using PropertyHeadPtr = std::shared_ptr<PropertyHead>;

    /// Convenience typedef for all connected property bodies, up- or downstream.
    using Connected = std::vector<valid_ptr<PropertyBody*>>;

public:
    NOTF_ACCESS_TYPES(SceneNode);

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag);

    /// Thrown when a Property head tries to access an already deleted PropertyGraph.
    NOTF_EXCEPTION_TYPE(no_graph);

    /// Checks if T is a valid type to store in a Property.
    template<typename T>
    using is_property_type = typename std::conjunction< //
        std::is_trivially_copyable<T>,                  // we need to copy property values to store them as deltas
        std::negation<std::is_const<T>>                 // property values must be modifyable
        >;                                              //
    template<typename T>
    static constexpr bool is_property_type_v = is_property_type<T>::value;

    // forwards
    class Batch;

    // ------------------------------------------------------------------------

    /// Virtual baseclass, so we can store updates of various property types in one `PropertyUpdates` list.
    struct Update {

        /// Constructor.
        /// @param target   Property targeted by this update.
        Update(PropertyHeadPtr target) : property(std::move(target)) {}

        /// Destructor.
        virtual ~Update();

        /// Property that was updated.
        PropertyHeadPtr property;
    };

    // ------------------------------------------------------------------------

    /// Helper type to store an update value for an untyped property.
    template<typename T>
    struct ValueUpdate final : public Update {

        /// Constructor.
        /// @param target   Property targeted by this update.
        /// @param value    New value of the targeted Property.
        ValueUpdate(PropertyHeadPtr target, T&& value) : Update(std::move(target)), value(std::forward<T>(value)) {}

        /// New value of the targeted Property.
        T value; // not const so we can move from it
    };

    // ------------------------------------------------------------------------

    /// Helper type to store an update expression for an untyped property.
    template<typename T>
    struct ExpressionUpdate final : public Update {

        /// Constructor.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Properties that the expression depends on.
        ExpressionUpdate(PropertyHeadPtr target, Expression<T>&& expression,
                         std::vector<PropertyHeadPtr>&& dependencies)
            : Update(std::move(target)), expression(std::forward<T>(expression)), dependencies(std::move(dependencies))
        {}

        /// New value of the targeted Property.
        Expression<T> expression; // not const so we can move from it

        /// Properties that the expression depends on.
        std::vector<PropertyHeadPtr> dependencies; // not const so we can move from it
    };

    // ------------------------------------------------------------------------

    /// Set of update to untyped properties.
    using UpdateSet = std::set<std::unique_ptr<Update>>;

    //=========================================================================
private:
    class PropertyBody {

        template<typename, typename>
        friend class TypedPropertyBody;

        // methods ------------------------------------------------------------
    public:
        /// Constructor.
        /// @param node     SceneNode associated with this Property.
        PropertyBody(const SceneNode* node) : m_node(node) {}

        /// Destructor.
        virtual ~PropertyBody();

        /// SceneNode associated with this Property (can be nullptr if this is an unassociated Property).
        /// @param graph        PropertyGraph containing this Property.
        risky_ptr<const SceneNode*> node(PropertyGraph& graph) const
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            return m_node;
        }

        /// Grounds this and all of its affected Properties.
        /// @param graph        PropertyGraph containing this Property.
        void prepare_removal(const PropertyGraph& graph);

    protected:
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param graph        PropertyGraph containing this Property.
        /// @param all_affected [OUT] Set of all associated properties affected of the change.
        virtual void _update(const PropertyGraph& graph, UpdateSet& all_affected) = 0;

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        /// @param graph    PropertyGraph containing this Property.
        void _ground(const PropertyGraph& graph);

        /// Checks that none of the proposed upstream Properties:
        ///     * is still alive (return false)
        ///     * is already downstream of this one (no_dag exception).
        ///     * has this one as a child already (assert).
        /// @param graph            PropertyGraph containing this Property.
        /// @param dependencies     Potential upstream connections to validate.
        /// @returns False if one of the dependencies has since been removed.
        bool _validate_upstream(const PropertyGraph& graph, const Connected& dependencies) const;

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        Connected m_upstream;

        /// Properties affected by this one through expressions.
        Connected m_downstream;

        /// SceneNode associated with this Property (can be nullptr if this is an unassociated Property).
        risky_ptr<const SceneNode*> m_node;
    };

    //=========================================================================
private:
    template<typename T, typename = std::enable_if_t<is_property_type_v<T>>>
    class TypedPropertyBody final : public PropertyBody {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the property type.
        /// @param node     SceneNode associated with this Property.
        TypedPropertyBody(T&& value, const SceneNode* node) : PropertyBody(node), m_value(std::forward(value)) {}

        /// The Property's value.
        /// @param graph    PropertyGraph containing this Property.
        const T& value(PropertyGraph& graph) const
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            return m_value;
        }

        /// Set the Property's value and updates downstream Properties.
        /// Removes an existing expression on this Property if one exists.
        /// @param graph        PropertyGraph containing this Property.
        /// @param value        New value.
        /// @param all_affected [OUT] Set of all associated properties affected of the change.
        void set_value(PropertyGraph& graph, T&& value, UpdateSet& all_affected)
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (m_expression) {
                _ground(graph);
            }
            _set_value(graph, std::forward<T>(value), all_affected);
        }

        /// Set the Property's expression.
        /// @param graph            PropertyGraph containing this Property.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
        void
        set_expression(PropertyGraph& graph, Expression<T> expression, Connected dependencies, UpdateSet& all_affected)
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (expression) {

                // always disconnect from the current upstream, even if the new dependencies might be invalid
                _ground(graph);
                if (!_validate_upstream(graph, dependencies)) {
                    return;
                }

                // update connections on yourself and upstream
                m_expression = std::move(expression);
                m_upstream = std::move(dependencies);
                for (valid_ptr<PropertyBody*> dependency : m_upstream) {
                    dependency->m_downstream.emplace_back(this);
                }

                // update the value of yourself and all downstream properties
                _update(graph, all_affected);
            }
        }

    private:
        /// Updates the Property by evaluating its expression.
        /// @param graph        PropertyGraph containing this Property.
        /// @param all_affected [OUT] Set of all associated properties affected of the change.
        void _update(const PropertyGraph& graph, UpdateSet& all_affected) override
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (m_expression) {
                _set_value(graph, m_expression(), all_affected);
            }
        }

        /// Changes the value of this Property if the new one is different and then updates all affected Properties.
        /// @param graph        PropertyGraph containing this Property.
        /// @param value        New value.
        /// @param all_affected [OUT] Set of all associated properties affected of the change.
        void _set_value(PropertyGraph& graph, T&& value, UpdateSet& all_affected)
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (value != m_value) {

                // order affected properties with upstream before downstream
                if (m_node) {
                    if (auto head = m_head.lock()) {
                        all_affected.emplace(std::make_unique<ValueUpdate<T>>(std::move(head), value));
                    }
                }

                // update the value of yourself and all downstream properties
                m_value = std::forward<T>(value);
                for (valid_ptr<PropertyBody*> affected : m_downstream) {
                    affected->_update(graph, all_affected);
                }
            }
        }

        /// Unassociates the SceneNode associated with the Property.
        /// @param graph    PropertyGraph containing this Property.
        void _unassociate(PropertyGraph& graph)
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            m_node = nullptr;
        }

        // fields -------------------------------------------------------------
    private:
        /// The head of this Property body.
        /// Is set by the `PropertyGraph::_create_property` factory.
        std::weak_ptr<Property<T>> m_head;

        /// Expression evaluating to a new value for this Property.
        Expression<T> m_expression;

        /// Value held by the Property.
        T m_value;
    };

    //=========================================================================
private:
    /// Base of all PropertyHead subclasses.
    /// Useful for storing a bunch of untyped PropertyHeads into a vector to pass as expression dependencies.
    class PropertyHead {

        template<typename T>
        friend class Property;

        friend class Batch;

        // methods ------------------------------------------------------------
    protected:
        /// Contructor.
        /// @param graph    PropertyGraph containing this Property's body.
        /// @param body     Body of the Property.
        PropertyHead(PropertyGraphPtr graph, valid_ptr<PropertyBody*> body)
            : m_graph(std::move(graph)), m_body(std::move(body))
        {}

    public:
        /// Destructor.
        virtual ~PropertyHead();

    protected:
        /// Checks if a given update would succeed if executed or not.
        /// @throws no_dag  If the update is an expression that would introduce a cyclic dependency into the graph.
        virtual void _validate_update(valid_ptr<Update*> update) = 0;

        /// Allows an untyped PropertyHead to ingest an untyped PropertyUpdate.
        /// Note that this method moves the value/expression out of the update.
        /// @param graph            Graph containing the properties to update.
        /// @param update           Update to apply.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        virtual void _apply_update(PropertyGraph& graph, valid_ptr<Update*> update, UpdateSet& all_affected) = 0;

        /// Returns the PropertyGraph managing this Property.
        /// If the graph has already gone out of scope, the nullptr is returned.
        risky_ptr<PropertyGraphPtr> _graph() const { return m_graph.lock(); }

        /// Fires a PropertyEvent targeting the given list of affected properties.
        /// @param affected     Associated properties affected
        void _fire_event(UpdateSet&& affected);

        // fields -------------------------------------------------------------
    protected:
        /// PropertyGraph containing the Property's body.
        std::weak_ptr<PropertyGraph> m_graph;

        /// Body of the Property.
        valid_ptr<PropertyBody*> m_body;
    };

    //=========================================================================
public:
    /// A Property Batch used to collect multiple property updates and execute them in a single transaction.
    /// Usable as a RAII object that automatically performs the transaction when going out of scope.
    /// Note however, that failures (like a "no_dag" error) will silently be ignored when used like that.
    /// If you are unsure if the batch will succeed, manually call `execute` after collecting all updates.
    class NOTF_NODISCARD Batch {

        friend class PropertyGraph;

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param graph    The PropertyGraph to update.
        Batch(PropertyGraphPtr&& graph) : m_graph(graph) {}

    public:
        NOTF_NO_COPY_OR_ASSIGN(Batch);

        /// Move Constructor.
        /// @param other    Other Batch to move from.
        Batch(Batch&& other) : m_graph(std::move(other.m_graph)), m_updates(std::move(other.m_updates)) {}

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
        template<typename T>
        void set_value(PropertyPtr<T> property, T&& value)
        {
            m_updates.emplace(std::make_unique<ValueUpdate<T>>(std::move(property), std::forward<T>(value)));
        }

        /// Set the Property's expression.
        /// Evaluates the expression right away to update the Property's value.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Properties that the expression depends on.
        template<typename T>
        void
        set_expression(PropertyPtr<T> property, Expression<T>&& expression, std::vector<PropertyHeadPtr>&& dependencies)
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
        /// The PropertyGraph to update.
        std::weak_ptr<PropertyGraph> m_graph;

        /// All updates that make up this batch.
        PropertyGraph::UpdateSet m_updates;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    PropertyGraph(SceneGraph& scene_graph) : m_scene_graph(scene_graph) {}

public:
    /// Factory method.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    static PropertyGraphPtr create(SceneGraph& scene_graph)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PropertyGraph, scene_graph);
    }

    /// Creates a new free Property in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    template<typename T>
    PropertyPtr<T> create_property(T&& value)
    {
        return _create_property(std::forward<T>(value));
    }

    /// Creates a new Batch.
    Batch create_batch() { return Batch(shared_from_this()); }

private:
    /// Creates a new Property in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    /// @param node     SceneNode associated with this Property.
    template<typename T>
    PropertyPtr<T> _create_property(T&& value, SceneNode* node = nullptr)
    {
        static_assert(is_property_type_v<T>, "T is not a valid Property type");
        auto body = std::make_unique<TypedPropertyBody<T>>(*this, std::forward<T>(value), node);
        auto head = Property<T>::create(shared_from_this(), body.get());
        body->m_head = head; // inject
        {
            std::lock_guard<Mutex> lock(m_mutex);
            m_properties.emplace(std::move(body));
        }
        return head;
    }

    /// Deletes a Property from the graph.
    /// @param property     Property to remove.
    void _delete_property(const valid_ptr<PropertyBody*> property)
    {
        std::lock_guard<Mutex> lock(m_mutex);
        auto it = m_properties.find(property);
        if (it != m_properties.end()) {
            (*it)->prepare_removal(*this);
            m_properties.erase(it);
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Mutex guarding the graph and all Properties in it.
    Mutex m_mutex;

    /// All Property bodies in this graph.
    std::set<std::unique_ptr<PropertyBody>, pointer_equal<PropertyBody>> m_properties;

    /// SceneGraph to deliver the PropertyEvents to.
    SceneGraph& m_scene_graph;
};

/// PropertyGraph access for SceneNodes.
template<>
class PropertyGraph::Access<SceneNode> {
    friend class SceneNode;

    /// Constructor.
    /// @param graph    PropertyGraph to access.
    Access(PropertyGraph& graph) : m_graph(graph) {}

    /// Creates a new Property node in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    /// @param node     SceneNode associated with this Property.
    template<typename T>
    PropertyHandler<T> create_property(T value, SceneNode* node)
    {
        return PropertyHandler<T>(m_graph._create_property(value, node), std::forward<T>(value));
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph to access.
    PropertyGraph& m_graph;
};

//====================================================================================================================//

/// PropertyHead to be used in the wild.
/// Is properly typed.
template<typename T>
class Property final : public PropertyGraph::PropertyHead {

    //    friend class PropertyGraph;

    //    template<typename, typename>
    //    friend class TypedPropertyBody;

    friend class PropertyHandler<T>;

    // types --------------------------------------------------------------
private:
    using Body = PropertyGraph::TypedPropertyBody<T>;
    using Connected = PropertyGraph::Connected;
    using ExpressionUpdate = PropertyGraph::ExpressionUpdate<T>;
    using PropertyHeadPtr = PropertyGraph::PropertyHeadPtr;
    using Update = PropertyGraph::Update;
    using UpdateSet = PropertyGraph::UpdateSet;
    using ValueUpdate = PropertyGraph::ValueUpdate<T>;

public:
    using Expression = PropertyGraph::Expression<T>;

    // methods ------------------------------------------------------------
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param graph    PropertyGraph containing this Property's body.
    /// @param body     Body of the Property.
    Property(PropertyGraphPtr graph, valid_ptr<Body*> body) : PropertyHead(std::move(graph), body) {}

    /// Factory method.
    /// @param graph    PropertyGraph containing this Property's body.
    /// @param body     Body of the Property.
    static PropertyPtr<T> create(PropertyGraphPtr graph, valid_ptr<Body*> body)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(Property, std::move(graph), body);
    }

public:
    /// The Property's value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    const T& value()
    {
        if (auto property_graph = _graph()) {
            std::unique_lock<Mutex> lock(property_graph->m_mutex);
            return _body()->value(*property_graph);
        }
        else {
            notf_throw(no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Set the Property's value and updates downstream Properties.
    /// Removes an existing expression on this Property if one exists.
    /// Fires a PropertyEvent targeting all affected properties.
    /// @param value        New value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    void set_value(T&& value) { _fire_event(_set_value(std::forward<T>(value))); }

    /// Set the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// Fires a PropertyEvent targeting all affected properties.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph        If the PropertyGraph has been deleted.
    void set_expression(Expression&& expression, std::vector<PropertyHeadPtr>&& dependencies)
    {
        _fire_event(_set_expression(std::move(expression), std::move(dependencies)));
    }

private:
    /// Checks if a given update would succeed if executed or not.
    /// @throws no_dag  If the update is an expression that would introduce a cyclic dependency into the graph.
    void _validate_update(valid_ptr<Update*> update) override
    {
        PropertyGraphPtr property_graph = _graph();
        NOTF_ASSERT(property_graph && property_graph->m_mutex.is_locked_by_this_thread());

        // check if an exception would introduce a cyclic dependency
        if (auto expression_update = dynamic_cast<ExpressionUpdate*>(update)) {
            _body()->_validate_upstream(*property_graph, _bodies_of_heads(expression_update->dependencies));
        }
    }

    /// Allows an untyped PropertyHead to ingest an untyped PropertyUpdate from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param graph            Graph containing the properties to update.
    /// @param update           Update to apply.
    /// @param all_affected     [OUT] Set of all associated properties affected of the change.
    void _apply_update(PropertyGraph& graph, valid_ptr<Update*> update, UpdateSet& all_affected) override
    {
        NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());

        // update with a ground value
        if (auto value_update = dynamic_cast<ValueUpdate*>(update)) {
            NOTF_ASSERT(value_update->property.get() == this);
            _body()->set_value(graph, std::forward<T>(value_update->value), all_affected);
        }

        // update with an expression
        else if (auto expression_update = dynamic_cast<ExpressionUpdate*>(update)) {
            NOTF_ASSERT(expression_update->property.get() == this);
            _body()->set_expression(graph, std::move(expression_update->expression),
                                    _bodies_of_heads(expression_update->dependencies), all_affected);
        }

        // should not happen
        else {
            NOTF_ASSERT(false);
        }
    }

    /// Updates a Property's value.
    /// Removes an existing expression on this Property if one exists.
    /// @param value        New value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    /// @returns            All Properties affected by the update with their new values.
    UpdateSet _set_value(T&& value)
    {
        if (auto property_graph = _graph()) {
            std::unique_lock<Mutex> lock(property_graph->m_mutex);

            UpdateSet affected;
            _body()->set_value(*property_graph, std::forward<T>(value), affected);
            return affected;
        }
        else {
            notf_throw(no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Method for PropertyHandler to update a Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// Does not fire an event.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph        If the PropertyGraph has been deleted.
    UpdateSet _set_expression(Expression&& expression, std::vector<PropertyHeadPtr>&& dependencies)
    {
        if (auto property_graph = _graph()) {
            std::unique_lock<Mutex> lock(property_graph->m_mutex);

            PropertyGraph::UpdateSet affected;
            _body()->set_expression(*property_graph, std::move(expression), _bodies_of_heads(dependencies), affected);
            return affected;
        }
        else {
            notf_throw(no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Unassociates the SceneNode associated with the Property.
    void _unassociate()
    {
        if (auto property_graph = _graph()) {
            std::unique_lock<Mutex> lock(property_graph->m_mutex);
            _body()->_unassociate(*property_graph);
        }
    }

    /// Type restoring access to the Property's body.
    valid_ptr<Body*> _body() const { return static_cast<Body*>(m_body.get()); }

    /// Transforms a list of property heads into a list of property bodies.
    static Connected _bodies_of_heads(const std::vector<PropertyHeadPtr>& heads)
    {
        Connected dependency_bodies;
        dependency_bodies.reserve(heads.size());
        std::transform(heads.begin(), heads.end(), dependency_bodies.begin(),
                       [](const PropertyHeadPtr& head) -> valid_ptr<PropertyGraph::PropertyBody*> {
                           return head->m_body;
                       });
        return dependency_bodies;
    }
};

//====================================================================================================================//

namespace detail {

/// Abstract base class allowing a SceneNode to match an untyped PropertyUpdate with the correct PropertyHandler.
struct PropertyHandlerBase {

    /// Destructor.
    virtual ~PropertyHandlerBase();

    /// Allows an untyped PropertyHandler to ingest an untyped PropertyUpdate.
    /// @param update   PropertyUpdate to apply.
    virtual void _apply_update(valid_ptr<PropertyGraph::Update*> update) = 0;
};

} // namespace detail

// -------------------------------------------------------------------------------------------------------------------//

template<typename T>
class PropertyHandler final : public detail::PropertyHandlerBase {

    friend class PropertyGraph::Access<SceneNode>;

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Fired when the value of the Property Handler changed.
    Signal<const T&> on_value_changed;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Constructor.
    /// @param property     Property to handle.
    /// @param value        Initial value of the Property.
    PropertyHandler(PropertyPtr<T>&& property, T value) : m_property(std::move(property)), m_value(std::move(value)) {}

public:
    /// Destructor.
    ~PropertyHandler() override
    {
        m_property->_unassociate();

        // delete the frozen value, if there is one
        if (T* frozen_ptr = m_frozen_value.exchange(nullptr)) {
            delete frozen_ptr;
        }
    }

    /// Last property value known to the handler.
    const T& value() const { return m_value; }

    // TODO: set
    // TODO: set expression

private:
    /// Updates the value in response to a PropertyEvent.
    /// @param update   PropertyUpdate to apply.
    void _apply_update(valid_ptr<PropertyGraph::Update*> update) override
    {
        PropertyGraph::ValueUpdate<T>* typed_update;
#ifdef NOTF_DEBUG
        typed_update = dynamic_cast<PropertyGraph::ValueUpdate<T>*>(update);
        NOTF_ASSERT(typed_update);
#else
        typed_update = static_cast<PropertyGraph::TypedPropertyUpdate<T>*>(update);
#endif
        if (typed_update->value != m_value) {
            m_value = std::move(typed_update->value);
            on_value_changed(m_value);
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Property to handle.
    PropertyPtr<T> m_property;

    /// Pointer to a frozen copy of the value, if it was modified while the SceneGraph was frozen.
    std::atomic<T*> m_frozen_value;

    /// Last property value known to the handler.
    T m_value;
};

NOTF_CLOSE_NAMESPACE
