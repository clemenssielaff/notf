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

namespace access { // forwards
template<class>
class _PropertyGraph;
class _PropertyGraph_Property;
class _PropertyGraph_PropertyHandler;
} // namespace access

//====================================================================================================================//

class PropertyGraph : public std::enable_shared_from_this<PropertyGraph> {

    // access ------------------------------------------------------------------------------------------------------- //

    friend class access::_PropertyGraph<SceneNode>;
    friend class access::_PropertyGraph<Window>;
    friend class access::_PropertyGraph_Property;
    friend class access::_PropertyGraph_PropertyHandler;
#ifdef NOTF_TEST
    friend class access::_PropertyGraph<test::Harness>;
#endif

    // types ---------------------------------------------------------------------------------------------------------//
private:
    // forwards
    class PropertyBody;
    class PropertyHead;
    class PropertyHandlerBase;
    template<class, class>
    class TypedPropertyBody;

    /// Convenience typedef for owning pointer to property heads.
    using PropertyHeadPtr = std::shared_ptr<PropertyHead>;

    /// Convenience typedef for all connected property bodies, up- or downstream.
    using Connected = std::vector<valid_ptr<PropertyBody*>>;

public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyGraph<T>;
    using PropertyAccess = access::_PropertyGraph_Property;
    using PropertyHandlerAccess = access::_PropertyGraph_PropertyHandler;

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag);

    /// Thrown when a Property head tries to access an already deleted PropertyGraph.
    NOTF_EXCEPTION_TYPE(no_graph);

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

    //=========================================================================

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
    template<class T>
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
    template<class T>
    struct ExpressionUpdate final : public Update {

        /// Constructor.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Properties that the expression depends on.
        ExpressionUpdate(PropertyHeadPtr target, Expression<T>&& expression,
                         std::vector<PropertyHeadPtr>&& dependencies)
            : Update(std::move(target))
            , expression(std::forward<Expression<T>>(expression))
            , dependencies(std::move(dependencies))
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
        template<class T>
        void set_value(PropertyPtr<T> property, T&& value)
        {
            m_updates.emplace(std::make_unique<ValueUpdate<T>>(std::move(property), std::forward<T>(value)));
        }

        /// Set the Property's expression.
        /// Evaluates the expression right away to update the Property's value.
        /// @param target           Property targeted by this update.
        /// @param expression       New expression for the targeted Property.
        /// @param dependencies     Properties that the expression depends on.
        template<class T>
        void set_expression(PropertyPtr<T> property, identity_t<Expression<T>>&& expression,
                            std::vector<PropertyHeadPtr>&& dependencies)
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

    //=========================================================================
private:
    class PropertyBody {

        template<class, class>
        friend class TypedPropertyBody;

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        virtual ~PropertyBody();

        /// Grounds this and all of its affected Properties.
        /// @param graph        PropertyGraph containing this Property.
        void prepare_removal(const PropertyGraph& graph);

        /// Checks that none of the proposed upstream Properties:
        ///     * is still alive (return false)
        ///     * is already downstream of this one (no_dag exception).
        ///     * has this one as a child already (assert).
        /// @param graph            PropertyGraph containing this Property.
        /// @param dependencies     Potential upstream connections to validate.
        /// @returns False if one of the dependencies has since been removed.
        bool validate_upstream(const PropertyGraph& graph, const Connected& dependencies) const;

    protected:
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        /// @param graph        PropertyGraph containing this Property.
        /// @param all_affected [OUT] Set of all associated properties affected of the change.
        virtual void _update(const PropertyGraph& graph, UpdateSet& all_affected) = 0;

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        /// @param graph    PropertyGraph containing this Property.
        virtual void _ground(const PropertyGraph& graph);

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        Connected m_upstream;

        /// Properties affected by this one through expressions.
        Connected m_downstream;
    };

    //=========================================================================
private:
    template<class T, typename = std::enable_if_t<is_property_type_v<T>>>
    class TypedPropertyBody final : public PropertyBody {

        friend class PropertyGraph;

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the property type.
        TypedPropertyBody(T&& value) : PropertyBody(), m_value(std::forward<T>(value)) {}

        /// Checks if the Property is grounded or not (has an expression).
        /// @param graph    PropertyGraph containing this Property.
        bool is_grounded(PropertyGraph& graph) const
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            return !static_cast<bool>(m_expression);
        }

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
                if (!validate_upstream(graph, dependencies)) {
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
        /// @param graph            PropertyGraph containing this Property.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        void _update(const PropertyGraph& graph, UpdateSet& all_affected) override
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (m_expression) {
                _set_value(graph, m_expression(), all_affected);
            }
        }

        /// Changes the value of this Property if the new one is different and then updates all affected Properties.
        /// @param graph            PropertyGraph containing this Property.
        /// @param value            New value.
        /// @param all_affected     [OUT] Set of all associated properties affected of the change.
        void _set_value(const PropertyGraph& graph, T&& value, UpdateSet& all_affected)
        {
            NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
            if (value != m_value) {

                // order affected properties upstream before downstream
                if (auto head = m_head.lock()) {
                    if (head->is_associated()) {
                        all_affected.emplace(std::make_unique<ValueUpdate<T>>(std::move(head), std::forward<T>(value)));
                    }
                }

                // update the value of yourself and all downstream properties
                m_value = std::forward<T>(value);
                for (valid_ptr<PropertyBody*> affected : m_downstream) {
                    affected->_update(graph, all_affected);
                }
            }
        }

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        /// @param graph    PropertyGraph containing this Property.
        void _ground(const PropertyGraph& graph) override
        {
            PropertyBody::_ground(graph);
            m_expression = {};
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

        template<class>
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

        /// Checks whether this Property is associated with a SceneNode or not.
        bool is_associated() const { return static_cast<bool>(m_handler); }

        risky_ptr<PropertyHandlerBase*> handler() { return m_handler; }

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

        // fields -------------------------------------------------------------
    protected:
        /// PropertyGraph containing the Property's body.
        std::weak_ptr<PropertyGraph> m_graph;

        /// Body of the Property.
        valid_ptr<PropertyBody*> m_body;

        /// Handler associated with this Property (can be nullptr if this is an unassociated Property).
        risky_ptr<PropertyHandlerBase*> m_handler;
    };

    //=========================================================================
private:
    /// Abstract base class allowing a SceneNode to match an untyped PropertyUpdate with the correct PropertyHandler.
    class PropertyHandlerBase {

        template<class>
        friend class PropertyHandler;

        // methods ------------------------------------------------------------
    protected:
        /// Constructor.
        /// @param node     SceneNode owning this Handler.
        PropertyHandlerBase(valid_ptr<SceneNode*> node) : m_node(node) {}

    public:
        /// Destructor.
        virtual ~PropertyHandlerBase();

        /// Checks if the SceneNode is frozen.
        /// @throws no_graph    If the SceneGraph of the node has been deleted.
        bool is_frozen() const;

        /// Checks if the SceneNode is frozen by a given thread.
        /// @param tread_id     Id of the thread to check for.
        /// @throws no_graph    If the SceneGraph of the node has been deleted.
        bool is_frozen_by(const std::thread::id thread_id) const;

    protected:
        /// Allows an untyped PropertyHandler to ingest an untyped PropertyUpdate.
        /// @param update   PropertyUpdate to apply.
        virtual void _apply_update(valid_ptr<PropertyGraph::Update*> update) = 0;

        // fields -------------------------------------------------------------
    protected:
        /// SceneNode owning this Handler.
        valid_ptr<SceneNode*> m_node;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    PropertyGraph(SceneGraph& scene_graph) : m_scene_graph(scene_graph) {}

    /// Factory method.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    static PropertyGraphPtr _create(SceneGraph& scene_graph)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PropertyGraph, scene_graph);
    }

public:
    /// Creates a new free Property in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    template<class T>
    PropertyPtr<T> create_property(T&& value)
    {
        static_assert(is_property_type_v<T>, "T is not a valid Property type");
        auto body = std::make_unique<TypedPropertyBody<T>>(std::forward<T>(value));
        auto head = Property<T>::create(shared_from_this(), body.get());
        body->m_head = head; // inject
        {
            std::lock_guard<RecursiveMutex> lock(m_mutex);
            m_properties.emplace(std::move(body));
        }
        return head;
    }

    /// Creates a new Batch.
    Batch create_batch() { return Batch(shared_from_this()); }

private:
    /// Deletes a Property from the graph.
    /// @param property     Property to remove.
    void _delete_property(const valid_ptr<PropertyBody*> property)
    {
        std::lock_guard<RecursiveMutex> lock(m_mutex);
        auto it = m_properties.find(property);
        if (it != m_properties.end()) {
            (*it)->prepare_removal(*this);
            m_properties.erase(it);
        }
    }

    /// Fires a PropertyEvent targeting the given list of affected properties.
    /// @param affected     Associated properties affected
    void _fire_event(UpdateSet&& affected);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Mutex guarding the graph and all Properties in it.
    /// Needs to be recursive because we need to execute user-defined expressions (that might lock the mutex) while
    /// already holding it.
    RecursiveMutex m_mutex;

    /// All Property bodies in this graph.
    std::set<std::unique_ptr<PropertyBody>, pointer_less_than> m_properties;

    /// SceneGraph to deliver the PropertyEvents to.
    SceneGraph& m_scene_graph;
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_PropertyGraph<SceneNode> {
    friend class notf::SceneNode;

    using PropertyHeadPtr = PropertyGraph::PropertyHeadPtr;

    /// Creates a new Property node in the graph.
    /// @param graph    PropertyGraph to operate on.
    /// @param value    Value held by the Property, is used to determine the property type.
    /// @param node     SceneNode associated with this Property.
    template<class T>
    static PropertyHandler<T> create_property(PropertyGraph& graph, T value, SceneNode* node)
    {
        return PropertyHandler<T>(graph.create_property(value), std::forward<T>(value), node);
    }
};

//-----------------------------------------------------------------------------

template<>
class access::_PropertyGraph<Window> {
    friend class notf::Window;

    /// Factory method.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    static PropertyGraphPtr create(SceneGraph& scene_graph) { return PropertyGraph::_create(scene_graph); }
};

//-----------------------------------------------------------------------------

class access::_PropertyGraph_Property {
    template<class>
    friend class notf::Property;

    template<class T>
    using TypedPropertyBody = PropertyGraph::TypedPropertyBody<T>;
    using PropertyBody = PropertyGraph::PropertyBody;
    using PropertyHead = PropertyGraph::PropertyHead;
    using PropertyHeadPtr = PropertyGraph::PropertyHeadPtr;
    using Connected = PropertyGraph::Connected;

    /// Direct access to the PropertyGraph's mutex.
    /// @param graph    PropertyGraph to operate on.
    static RecursiveMutex& mutex(PropertyGraph& graph) { return graph.m_mutex; }

    /// Fires a PropertyEvent targeting the given list of affected properties.
    /// @param affected     Associated properties affected
    static void fire_event(PropertyGraph& graph, PropertyGraph::UpdateSet&& affected)
    {
        graph._fire_event(std::move(affected));
    }
};

//-----------------------------------------------------------------------------

class access::_PropertyGraph_PropertyHandler {
    template<class>
    friend class notf::PropertyHandler;

    using PropertyHandlerBase = PropertyGraph::PropertyHandlerBase;
    template<class T>
    using Expression = PropertyGraph::Expression<T>;
    using PropertyHeadPtr = PropertyGraph::PropertyHeadPtr;

    /// Direct access to the PropertyGraph's mutex.
    /// @param graph    PropertyGraph to operate on.
    static RecursiveMutex& mutex(PropertyGraph& graph) { return graph.m_mutex; }
};
//====================================================================================================================//

/// PropertyHead to be used in the wild.
/// Is properly typed.
template<class T>
class Property final : public PropertyGraph::PropertyAccess::PropertyHead {

    friend class PropertyGraph;

    // types --------------------------------------------------------------
private:
    using PropertyBody = PropertyGraph::PropertyAccess::PropertyBody;
    using Body = PropertyGraph::PropertyAccess::TypedPropertyBody<T>;
    using Connected = PropertyGraph::PropertyAccess::Connected;
    using PropertyHeadPtr = PropertyGraph::PropertyAccess::PropertyHeadPtr;

    using ExpressionUpdate = PropertyGraph::ExpressionUpdate<T>;
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
    /// Checks if the Property is grounded or not (has an expression).
    bool is_grounded() const
    {
        if (auto property_graph = _graph()) {
            NOTF_MUTEX_GUARD(PropertyGraph::PropertyAccess::mutex(*property_graph));
            return _body()->is_grounded(*property_graph);
        }
        else {
            notf_throw(PropertyGraph::no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Checks if the Property has an expression or not (is grounded).
    bool has_expression() const { return !is_grounded(); }

    /// The Property's value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    const T& value()
    {
        if (auto property_graph = _graph()) {
            NOTF_MUTEX_GUARD(PropertyGraph::PropertyAccess::mutex(*property_graph));
            return _body()->value(*property_graph);
        }
        else {
            notf_throw(PropertyGraph::no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Set the Property's value and updates downstream Properties.
    /// Removes an existing expression on this Property if one exists.
    /// Fires a PropertyEvent targeting all affected properties.
    /// @param value        New value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    void set_value(T&& value) { _set_value(std::forward<T>(value), /* fire_event = */ true); }

    /// Set the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// Fires a PropertyEvent targeting all affected properties.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph        If the PropertyGraph has been deleted.
    void set_expression(Expression&& expression, std::vector<PropertyHeadPtr>&& dependencies)
    {
        _set_expression(std::move(expression), std::move(dependencies), /* fire_event = */ true);
    }

private:
    /// Checks if a given update would succeed if executed or not.
    /// @throws no_dag  If the update is an expression that would introduce a cyclic dependency into the graph.
    void _validate_update(valid_ptr<Update*> update) override
    {
        PropertyGraphPtr property_graph = _graph();
        NOTF_ASSERT(property_graph && PropertyGraph::PropertyAccess::mutex(*property_graph).is_locked_by_this_thread());

        // check if an exception would introduce a cyclic dependency
        if (auto expression_update = dynamic_cast<ExpressionUpdate*>(update.get())) {
            _body()->validate_upstream(*property_graph, _bodies_of_heads(expression_update->dependencies));
        }
    }

    /// Allows an untyped PropertyHead to ingest an untyped PropertyUpdate from a Batch.
    /// Note that this method moves the value/expression out of the update.
    /// @param graph            Graph containing the properties to update.
    /// @param update           Update to apply.
    /// @param all_affected     [OUT] Set of all associated properties affected of the change.
    void _apply_update(PropertyGraph& graph, valid_ptr<Update*> update, UpdateSet& all_affected) override
    {
        NOTF_ASSERT(PropertyGraph::PropertyAccess::mutex(graph).is_locked_by_this_thread());

        // update with a ground value
        if (auto value_update = dynamic_cast<ValueUpdate*>(update.get())) {
            NOTF_ASSERT(value_update->property.get() == this);
            _body()->set_value(graph, std::forward<T>(value_update->value), all_affected);
        }

        // update with an expression
        else if (auto expression_update = dynamic_cast<ExpressionUpdate*>(update.get())) {
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
    /// @param fire_event   Whether the PropertyGraph should fire off a PropertyEvent in response to the change.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    /// @returns            All Properties affected by the update with their new values if `fire_event` is false.
    ///                     If `fire_event` is true, the returned value will always be empty.
    UpdateSet _set_value(T&& value, const bool fire_event)
    {
        if (auto property_graph = _graph()) {
            NOTF_MUTEX_GUARD(PropertyGraph::PropertyAccess::mutex(*property_graph));

            UpdateSet affected;
            _body()->set_value(*property_graph, std::forward<T>(value), affected);

            if (fire_event && !affected.empty()) {
                PropertyGraph::PropertyAccess::fire_event(*property_graph, std::move(affected));
                affected.clear();
            }
            return affected;
        }
        else {
            notf_throw(PropertyGraph::no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Method for PropertyHandler to update a Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// Does not fire an event.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @param fire_event       Whether the PropertyGraph should fire off a PropertyEvent in response to the change.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph        If the PropertyGraph has been deleted.
    /// @returns                All Properties affected by the update with their new values if `fire_event` is false.
    ///                         If `fire_event` is true, the returned value will always be empty.
    UpdateSet
    _set_expression(Expression&& expression, std::vector<PropertyHeadPtr>&& dependencies, const bool fire_event)
    {
        if (auto property_graph = _graph()) {
            NOTF_MUTEX_GUARD(PropertyGraph::PropertyAccess::mutex(*property_graph));

            PropertyGraph::UpdateSet affected;
            _body()->set_expression(*property_graph, std::move(expression), _bodies_of_heads(dependencies), affected);

            if (fire_event && !affected.empty()) {
                PropertyGraph::PropertyAccess::fire_event(*property_graph, std::move(affected));
                affected.clear();
            }
            return affected;
        }
        else {
            notf_throw(PropertyGraph::no_graph, "PropertyGraph has been deleted");
        }
    }

    /// Unassociates the SceneNode associated with the Property.
    void _unassociate() { m_handler = nullptr; }

    /// Type restoring access to the Property's body.
    valid_ptr<Body*> _body() const { return static_cast<Body*>(m_body.get()); }

    /// Transforms a list of property heads into a list of property bodies.
    static Connected _bodies_of_heads(const std::vector<PropertyHeadPtr>& heads)
    {
        Connected dependency_bodies;
        dependency_bodies.reserve(heads.size());
        std::transform(heads.begin(), heads.end(), std::back_inserter(dependency_bodies),
                       [](const PropertyHeadPtr& head) -> valid_ptr<PropertyBody*> { return head->m_body; });
        return dependency_bodies;
    }
};

//====================================================================================================================//

template<class T>
class PropertyHandler final : public PropertyGraph::PropertyHandlerAccess::PropertyHandlerBase {

    friend class access::_PropertyGraph<SceneNode>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using PropertyHandlerBase = PropertyGraph::PropertyHandlerAccess::PropertyHandlerBase;
    using PropertyHeadPtr = PropertyGraph::PropertyHandlerAccess::PropertyHeadPtr;
    using Expression = PropertyGraph::Expression<T>;

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Fired when the value of the Property Handler changed.
    Signal<const T&> on_value_changed;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Constructor.
    /// @param property     Property to handle.
    /// @param value        Initial value of the Property.
    /// @param node         SceneNode owning this Handler.
    PropertyHandler(PropertyPtr<T>&& property, T value, valid_ptr<SceneNode*> node)
        : PropertyHandlerBase(node), m_property(std::move(property)), m_value(std::move(value))
    {}

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
    /// @throws no_graph    If the PropertyGraph has been deleted.
    const T& value() const
    {
        // if the property is frozen by this thread (the render thread, presumably) and there exists a frozen copy of
        // the value, use that instead of the current one
        if (m_property->_is_frozen_by(std::this_thread::get_id())) {
            if (T* frozen_value = m_frozen_value.load(std::memory_order_consume)) {
                return *frozen_value;
            }
        }
        return m_value;
    }

    /// Set the Property's value.
    /// Removes an existing expression on this Property if one exists.
    /// @param value        New value.
    /// @throws no_graph    If the PropertyGraph has been deleted.
    void set_value(T&& value)
    {
        PropertyGraph::UpdateSet updates = m_property->_set_value(std::forward<T>(value), /* fire_event = */ false);
        _update_affected(std::move(updates));
    }

    /// Sets the Property's expression.
    /// Evaluates the expression right away to update the Property's value.
    /// @param expression       Expression to set.
    /// @param dependencies     Properties that the expression depends on.
    /// @throws no_dag          If the expression would introduce a cyclic dependency into the graph.
    /// @throws no_graph        If the PropertyGraph has been deleted.
    void _set_expression(Expression&& expression, std::vector<PropertyHeadPtr>&& dependencies)
    {
        PropertyGraph::UpdateSet updates
            = m_property->_set_expression(std::move(expression), std::move(dependencies), /* fire_event = */ false);
        _update_affected(std::move(updates));
    }

    /// The Property head contained in this Handler.
    Property<T> property() const { return m_property; }

private:
    /// Shallow update of affected PropertyHandlers
    /// @param affected     Other PropertyHandlers affected by the
    void _update_affected(PropertyGraph::UpdateSet&& affected)
    {
        for (const auto& update : affected) {
            risky_ptr<PropertyHandlerBase*> affected_handler = update->property->handler();
            NOTF_ASSERT(affected_handler);
            affected_handler->_apply_update(update.get());
        }
    }

    /// Updates the value in response to a PropertyEvent.
    /// @param update   PropertyUpdate to apply.
    void _apply_update(valid_ptr<PropertyGraph::Update*> update) override
    {
        // restore the correct update type
        PropertyGraph::ValueUpdate<T>* typed_update;
#ifdef NOTF_DEBUG
        typed_update = dynamic_cast<PropertyGraph::ValueUpdate<T>*>(update);
        NOTF_ASSERT(typed_update);
#else
        typed_update = static_cast<PropertyGraph::TypedPropertyUpdate<T>*>(update);
#endif

        if (typed_update->value == m_value) {
            return; // early out
        }

        // if the property is currently frozen and this is the first modification, create a frozen copy of the current
        // value before changing it
        if (m_property->_is_frozen()) {
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
    /// Property to handle.
    PropertyPtr<T> m_property;

    /// Pointer to a frozen copy of the value, if it was modified while the SceneGraph was frozen.
    std::atomic<T*> m_frozen_value;

    /// Last property value known to the handler.
    T m_value;
};

NOTF_CLOSE_NAMESPACE
