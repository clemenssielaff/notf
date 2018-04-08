#pragma once

#include "app/application.hpp"
#include "app/io/time.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thread-safe container of arbitrary values that can be connected using expressions.
///
/// Property
/// ========
///
/// A `Property` in NoTF is a container class for a single unit of *self-contained* data. The application-wide `time`
/// and `framenumber` are Properties, so are widget transformations, progress bar percentages or whatever else you
/// need. An integer would be a prime example, so would be a boolean. A node in a linked list would not qualify as
/// Property, because it would need to link to other Nodes and would therefore not be self-contained. Strings on the
/// other hand do qualify, because even though they contain a pointer to the heap, they are conceptually self-contained.
/// Note that NoTF cannot enforce that each Property type is indeed *self-contained*, use your own judgement.
///
/// There are two reason for why Properties are so prevalent:
///
/// 1. They can be used to build *Property Expressions* in the `PropertyGraph`.
/// 2. They can safely be used from multiple threads and can be used to *freeze* the global state during rendering
/// without putting any responsibility on the user to do so.
///
/// You can read Properties from both the render thread and all other threads, however when you read from any thread but
/// the render thread, Properties can change at any time. Each individual access is protected by a mutex, but if you
/// have a Property that is set a thousand times per second by an animation, you might read a different value at the
/// start of a long-running function then you'd get at the end.
///
/// The Property Graph
/// ==================
///
/// The Property Graph is the central management instance for all Properties in NoTF. It owns the Property `nodes`, the
/// data that makes up a Property itself, while the `Property<T>` *access* types live in userland.
///
/// *Freezing* the Property Graph
/// =============================
///
/// Conceptually we have this:
///
/// ```
///  Events   Timer    Commands   ...       Renderer
///     ^       ^         ^        ^           ^
///     |       |         |        |           |
///     |       +---+ +---+        |           |
///     +---------+ | | +----------+           |
///          read | | | | write                |
///               v v v v                      |
///           +-------------+                  |
///           |    Delta    |                  |
///           +------+------+             read |
///                  | resolve                 |
///           +------v-------------------------+------+
///           |             Ground Truth              |
///           +---------------------------------------+
/// ```
///
/// Everybody but the RenderManager has read/write access to a conceptual Delta that sits on top of the Ground Truth,
/// while the RenderManager uses the Ground Truth to render a frozen state of the UI. Whenever the RenderManager is
/// about to render a new frame, it commits the changes from the Delta to the Ground Truth (which is blocking so that no
/// further changes can be made to the Delta) and then releases all locks on the Delta again. This should allow for
/// minimal blockage and maximal responsiveness.
///
class PropertyGraph {

    // TODO: implement proper time handling for properties (we might have to re-introduce the dedicated dirty flag)

    // friends
    friend class detail::PropertyBase;
    template<typename>
    friend class Property;

    // forwards
    template<typename>
    class NodeDelta;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(test::Harness)

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error)

    /// Alias for checking if T is a valid type to store in a Property.
    template<typename T>
    using is_property_type = typename std::conjunction<std::is_copy_constructible<T>, std::negation<std::is_const<T>>>;

    //=========================================================================
private:
    /// Untyped base type for all PropertyNode types.
    /// It is required (but unchecked) that you hold the PropertyGraph mutex for all operations on PropertyNodes.
    class NodeBase {

        // methods ------------------------------------------------------------
    public:
        /// Virtual destructor.
        virtual ~NodeBase();

        /// Creates a properly typed delta copy of this node.
        virtual std::unique_ptr<NodeBase> copy_to_delta() = 0;

        /// Resolves the modification delta back into this node.
        virtual void resolve_delta(valid_ptr<NodeBase*> delta) = 0;

        /// Id of this PropertyNode. Is simply `this` unless this Node is part of a Delta.
        virtual valid_ptr<NodeBase*> id() { return this; }

    protected:
        /// Checks if this Property is dirty.
        bool _is_dirty() const { return m_time.is_invalid(); }

        /// Sets the Property dirty.
        void _set_dirty() { m_time = Time::invalid(); }

        /// Sets the Property clean.
        /// @param time     Time when the decision to set the Property was made. Is `now` by default.
        void _set_clean(Time time = Time::invalid()) { m_time = (time.is_valid() ? std::move(time) : Time::now()); }

        /// Removes the expression from this Property.
        virtual void _clear_expression() = 0;

        /// Grounding a property means removing its expression without changing its value.
        void _ground()
        {
            _clear_expression();
            _unregister_from_dependencies();
            _set_clean();
        }

        /// Registers this property as affected with all of its dependencies.
        /// @returns False iff any of the dependencies was deleted.
        bool _register_with_dependencies()
        {
            valid_ptr<NodeBase*> const my_id = id();
            for (valid_ptr<NodeBase*> dependency_id : m_dependencies) {
                risky_ptr<NodeBase*> dependency = PropertyGraph::instance().write_node(dependency_id);
                if (!dependency) {
                    return false;
                }
                NOTF_ASSERT(std::find(dependency->m_affected.begin(), dependency->m_affected.end(), my_id)
                            == dependency->m_affected.end());
                dependency->m_affected.emplace_back(my_id);
            }
            return true;
        }

        /// Removes all dependencies from this property.
        void _unregister_from_dependencies()
        {
            for (valid_ptr<NodeBase*> dependency_id : m_dependencies) {
                risky_ptr<NodeBase*> dependency = PropertyGraph::instance().write_node(dependency_id);
                if (!dependency) {
                    continue; // ignore
                }
                auto it = std::find(dependency->m_affected.begin(), dependency->m_affected.end(), id());
                NOTF_ASSERT(it != dependency->m_affected.end());
                *it = std::move(dependency->m_affected.back());
                dependency->m_affected.pop_back();
            }
            m_dependencies.clear();
        }

        /// Set all affected properties dirty.
        void _set_affected_dirty()
        {
            for (valid_ptr<NodeBase*> affected_id : m_affected) {
                if (risky_ptr<NodeBase*> affected = PropertyGraph::instance().write_node(affected_id)) {
                    affected->_set_dirty();
                }
            }
        }

        /// Detects potential cyclic dependencies in the graph before they are introduced.
        /// @param dependencies                 All PropertyNodes that this node will potentially be dependent on.
        /// @throws no_dag_error     If the expression would introduce a cyclic dependency into the graph.
        void _detect_cycles(const std::vector<valid_ptr<NodeBase*>>& dependencies);

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        std::vector<valid_ptr<NodeBase*>> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<valid_ptr<NodeBase*>> m_affected;

        /// Time when this Property was last defined.
        /// Is invalid if the Property is dirty (its expression needs to be evaluated).
        Time m_time = Time::now();
    };

    //=========================================================================

    /// A typed PropertyNode in the Property graph that manages all of the Properties connection, its value and optional
    /// expression.
    template<typename T, typename = std::enable_if_t<is_property_type<T>::value>>
    class Node : public NodeBase {

        template<typename>
        friend class NodeDelta;

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        Node(T value) : NodeBase(), m_value(std::move(value)) {}

        /// Creates a properly typed delta copy of this PropertyNode instance.
        virtual std::unique_ptr<NodeBase> copy_to_delta() override { return std::make_unique<NodeDelta<T>>(*this); }

        /// Resolves the modification delta back into this node.
        virtual void resolve_delta(valid_ptr<NodeBase*> node) override
        {
            valid_ptr<NodeDelta<T>*> delta = static_cast<NodeDelta<T>*>(node.get());
            m_dependencies = delta->m_dependencies;
            m_affected = delta->m_affected;
            m_time = delta->m_time;
            m_expression = delta->m_expression;
            m_value = delta->m_value;
        }

        /// The Property's value.
        /// If the Property is defined by an expression, this might evaluate the expression.
        const T& value()
        {
            if (_is_dirty()) {
                _evaluate_expression();
            }
            return m_value;
        }

        /// Set the Property's value.
        /// Removes an existing expression on the Property if it exists.
        /// @param value    New value.
        void set_value(T&& value)
        {
            if (m_expression) {
                _ground();
            }
            if (value == m_value) {
                return;
            }
            m_value = std::forward<T>(value);
            _set_affected_dirty();
        }

        /// Set the Property's expression.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
        void set_expression(std::function<T()> expression, std::vector<valid_ptr<NodeBase*>> dependencies)
        {
            if (!expression) {
                return _ground();
            }
            _detect_cycles(dependencies);

            _unregister_from_dependencies();
            m_expression = std::move(expression);
            m_dependencies = std::move(dependencies);
            if (!_register_with_dependencies()) {
                return _ground();
            }

            _set_dirty();
            _set_affected_dirty();
        }

    private:
        /// Removes the expression from this Property.
        void _clear_expression() override
        {
            if (m_expression) {
                _evaluate_expression();
                m_expression = {};
            }
        }

        /// Evaluates this Property's expression and updates its value.
        void _evaluate_expression()
        {
            NOTF_ASSERT_MSG(m_expression, "Cannot evaluate expression from ground Property: \"" << this << "\"");
            m_value = m_expression();
            _set_clean();
        }

        // fields -------------------------------------------------------------
    protected:
        /// Expression evaluating to a new value for this Property.
        std::function<T()> m_expression;

        /// Value held by the Property.
        T m_value;
    };

    //=========================================================================

    /// Subclass of a PropertyNode to be used in a modification Delta.
    /// Carries the id of the copied node with it.
    template<typename T>
    class NodeDelta : public Node<T> {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param other    Node to copy.
        NodeDelta(Node<T>& other) : Node<T>(other.m_value), m_id(&other)
        {
            Node<T>::m_dependencies = other.m_dependencies;
            Node<T>::m_affected = other.m_affected;
            Node<T>::m_time = other.m_time;
            Node<T>::m_expression = other.m_expression;
        }

        /// Destructor.
        /// Makes sure that the NodeDelta doesn't touch any other nodes on deletion.
        virtual ~NodeDelta() override
        {
            Node<T>::m_dependencies.clear();
            Node<T>::m_affected.clear();
        }

        /// Id of the copied PropertyNode.
        virtual valid_ptr<NodeBase*> id() { return m_id; }

        /// Creates a properly typed delta copy of this PropertyNode instance.
        virtual std::unique_ptr<NodeBase> copy_to_delta() override
        {
            notf_throw(internal_error, "Cannot copy a NodeDelta into a new delta");
        }

        /// Resolves the modification delta back into this node.
        virtual void resolve_delta(valid_ptr<NodeBase*>) override
        {
            notf_throw(internal_error, "Cannot copy into an existing NodeDelta");
        }

        // fields -------------------------------------------------------------
    private:
        /// Id of the copied PropertyNode.
        valid_ptr<NodeBase*> const m_id;
    };

    //=========================================================================
    //=========================================================================

    /// Virtual base class for all Delta types.
    struct DeltaBase {
        /// Destructor.
        virtual ~DeltaBase();

        /// Returns true iff this Delta is a modification Delta, false if it is a deletion Delta.
        virtual bool is_modification() const = 0;

        /// The PropertyNode referred to by this Delta.
        virtual valid_ptr<NodeBase*> node() const = 0;
    };

    //=========================================================================

    /// Delta used to store information about a modified Property.
    struct ModificationDelta : public DeltaBase {

        /// Default / value constructor.
        /// @param value    Value of the modified Property.
        ModificationDelta(std::unique_ptr<NodeBase>&& node) : m_node(std::move(node)) {}

        /// Destructor.
        virtual ~ModificationDelta() override;

        /// This IS a modification delta.
        virtual bool is_modification() const override { return true; }

        /// The modified PropertyNode.
        virtual valid_ptr<NodeBase*> node() const override { return m_node.get(); }

        // fields -------------------------------------------------------------
    private:
        /// Copy of the modified node.
        std::unique_ptr<NodeBase> m_node;
    };

    //=========================================================================

    /// Delta used to store information about a deleted Property.
    struct DeletionDelta : public DeltaBase {

        /// Value constructor.
        /// @param node Node of the deleted Property.
        DeletionDelta(NodeBase* node) : m_node(node) {}

        /// Destructor.
        virtual ~DeletionDelta() override;

        /// This is a deletion delta.
        virtual bool is_modification() const override { return false; }

        /// The modified PropertyNode.
        virtual valid_ptr<NodeBase*> node() const override { return m_node; }

        // fields -------------------------------------------------------------
    private:
        /// Property that was deleted.
        valid_ptr<NodeBase*> m_node;
    };

    //=========================================================================

    /// RAII object to make sure that a frozen graph is ALWAYS unfrozen again
    struct NOTF_NODISCARD FreezeGuard {
        /// Constructor.
        FreezeGuard() { PropertyGraph::instance().freeze(); }

        /// Destructor.
        ~FreezeGuard() { PropertyGraph::instance().unfreeze(); }
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph singleton.
    /// @throws application_initialization_error    If you call this method before an Application has been initialized.
    static PropertyGraph& instance() { return Application::instance().property_graph(); }

    /// Creates a new Property node in the graph.
    /// @param value    First ground value of the Property, is used to determine its type.
    template<typename T>
    valid_ptr<NodeBase*> create_node(T&& value)
    {
        std::unique_ptr<NodeBase> node = std::make_unique<Node<T>>(std::forward<T>(value));
        NodeBase* id = node.get();
        {
            std::lock_guard<RecursiveMutex> lock(m_mutex);
            m_nodes.emplace(std::make_pair(id, std::move(node)));
        }
        return id;
    }

    /// Returns a PropertyNode for reading without creating a new delta.
    /// @param node         Property node to read from.
    /// @param thread_id    ID of the freezing thread, uses the calling thread by default. (exposed for testability)
    /// @returns            The requested node or nullptr, if the node has a deletion delta.
    risky_ptr<NodeBase*>
    read_node(valid_ptr<NodeBase*> node, const std::thread::id thread_id = std::this_thread::get_id())
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen() || is_render_thread(thread_id)) {
            return node; // direct access
        }

        // return the frozen node if there is no delta for it
        auto it = m_delta.find(node);
        if (it == m_delta.end()) {
            return node;
        }

        // return an existing modification delta
        const std::unique_ptr<DeltaBase>& delta = it->second;
        if (delta->is_modification()) {
            return delta->node();
        }
        return nullptr;
    }

    /// Returns a PropertyNode for writing.
    /// Creates a new NodeDelta if the graph is frozen.
    /// @param node     Property node to write into.
    /// @returns        The requested Node or nullptr, if the node already has a deletion delta.
    risky_ptr<NodeBase*> write_node(valid_ptr<NodeBase*> node)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen()) {
            return node; // direct access
        }
        if (is_render_thread(std::this_thread::get_id())) {
            return nullptr; // do nothing if this is the render thread resolving the delta
        }

        // return an existing modification delta
        auto it = m_delta.find(node);
        if (it != m_delta.end()) {
            if (it->second->is_modification()) {
                return it->second->node();
            }
            return nullptr; // node has a deletion delta
        }

        // create a new modification delta
        auto new_delta = std::make_unique<ModificationDelta>(node->copy_to_delta());
        auto result = new_delta->node();
        m_delta.emplace(std::make_pair(node, std::move(new_delta)));
        return result;
    }

    /// Deletes a given Property node.
    /// Gives the Graph the chance to create a deletion delta for a deleted node.
    /// @param node         Property node to delete.
    /// @param thread_id    Id of the thread calling this function (exposed for testability).
    void delete_node(valid_ptr<NodeBase*> node, const std::thread::id thread_id = std::this_thread::get_id())
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());

        // delete the property node straight away if the graph isn't frozen
        // or if this is the render thread resolving the delta
        if (!is_frozen() || is_render_thread(thread_id)) {
            auto it = m_nodes.find(node);
            NOTF_ASSERT_MSG(it != m_nodes.end(), "Cannot delete unknown Property Node");
            m_nodes.erase(it);
            return;
        }

        // if the graph is frozen and the property is deleted, create a new deletion delta
        auto it = m_delta.find(node);
        if (it == m_delta.end()) {
            m_delta.emplace(std::make_pair(node, std::make_unique<DeletionDelta>(node)));
            return;
        }

        // if there is already a modification delta in place, replace it with a deletion delta
        NOTF_ASSERT_MSG(it->second->is_modification(), "Cannot delete the same Node twice");
        std::unique_ptr<DeltaBase> delete_delta = std::make_unique<DeletionDelta>(node);
        it.value().swap(delete_delta);
    }

    /// Checks if the PropertyGraph is currently frozen or not.
    bool is_frozen() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_render_thread != std::thread::id();
    }

    /// Checks if the calling thread is the current render thread.
    bool is_render_thread(const std::thread::id thread_id)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return thread_id == m_render_thread;
    }

    /// Freezes the PropertyGraph.
    /// All subsequent Property modifications and removals will create Delta objects until the Delta is resolved.
    /// Is ignored if the graph is already frozen.
    /// @param thread_id    ID of the freezing thread, uses the calling thread by default. (exposed for testability)
    void freeze(const std::thread::id thread_id = std::this_thread::get_id());

    /// Unfreezes the PropertyGraph and resolves all deltas.
    /// @param thread_id        ID of the freezing thread, uses the calling thread by default. (exposed for testability)
    /// @throws thread_error    If any but the freezing thread tries to unfreeze the graph.
    void unfreeze(const std::thread::id thread_id = std::this_thread::get_id());

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All nodes managed by the graph.
    robin_map<valid_ptr<NodeBase*>, std::unique_ptr<NodeBase>, PointerHash<NodeBase>> m_nodes;

    /// The current delta.
    robin_map<valid_ptr<NodeBase*>, std::unique_ptr<DeltaBase>, PointerHash<NodeBase>> m_delta;

    /// Mutex guarding the graph.
    /// Has to be recursive because Properties with expressions require other Properties to call their `value` method,
    /// which requires the mutex to be locked.
    mutable RecursiveMutex m_mutex;

    /// Thread id of the renderer thread, if it currently rendering.
    /// Also used as a flag whether the graph currently has a Delta or not.
    std::thread::id m_render_thread;
};

// ===================================================================================================================//

namespace detail {

/// Base class of all Properties.
/// Necessary so you can collect Properties of various types together in a vector of dependencies for `set_expression`.
class PropertyBase {
    template<typename T>
    friend class notf::Property;

    /// Convenience typedef.
    using NodeBase = PropertyGraph::NodeBase;

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// (somewhat) Owning pointer to the node of this Property in the graph.
    /// I say somewhat because ultimately it is the graph that owns the Property node, but this is the only reference to
    /// it on the outside.
    risky_ptr<NodeBase*> m_node = nullptr;
};

} // namespace detail

// ===================================================================================================================//

/// An object managing a single Property in the PropertyGraph.
/// Contains a unique_ptr to a PropertyNode in the graph.
template<typename T>
class Property : public detail::PropertyBase {

    /// Typed Property node type.
    using Node = PropertyGraph::Node<T>;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Property)

    /// Constructor.
    /// @param value    Initial value of the Property.
    explicit Property(T&& value) : PropertyBase()
    {
        m_node = PropertyGraph::instance().create_node(std::forward<T>(value));
    }

    /// Move Constructor.
    /// @param other    Property to move from.
    Property(Property&& other)
    {
        m_node = other.m_node;
        other.m_node = nullptr;
    }

    /// Destructor.
    ~Property()
    {
        if (!m_node) {
            return;
        }
        if (Application::was_closed()) {
            return;
        }
        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);
            graph.delete_node(m_node);
        }
        m_node = nullptr;
    }

    /// The value of this Property.
    const T& value() const
    {
        NOTF_ASSERT_MSG(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);

            // we wouldn't be here if the Property had been removed
            valid_ptr<Node*> node = static_cast<Node*>(graph.read_node(m_node).get());
            return node->value();
        }
    }

    /// Sets the value of this Property.
    /// @param value    New value.
    void set_value(T&& value)
    {
        NOTF_ASSERT_MSG(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);

            // we wouldn't be here if the Property had been removed
            valid_ptr<Node*> node = static_cast<Node*>(graph.write_node(m_node).get());
            node->set_value(std::forward<T>(value));
        }
    }

    /// Sets an expression of this Property.
    /// @param expression       New expression defining the value of the Property.
    /// @param dependency       All Property dependencies. It is of critical importance that all Properties in the
    ///                         expression are listed.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(std::function<T()>&& expression,
                        const std::vector<std::reference_wrapper<detail::PropertyBase>>& dependency)
    {
        NOTF_ASSERT_MSG(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        // collect the set of pointers from the expression dependencies
        std::vector<valid_ptr<NodeBase*>> dependencies;
        dependencies.reserve(dependencies.size());
        for (const auto& ref : dependency) {
            valid_ptr<NodeBase*> ptr = ref.get().m_node;
            if (std::find(dependencies.begin(), dependencies.end(), ptr) == dependencies.end()) {
                dependencies.emplace_back(ptr);
            }
        }

        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);

            // we wouldn't be here if the Property had been removed
            valid_ptr<Node*> node = static_cast<Node*>(graph.write_node(m_node).get());
            node->set_expression(std::move(expression), std::move(dependencies));
        }
    }

#ifdef NOTF_TEST
    /// Direct access to the node (id) of this Property - for testability only!
    risky_ptr<NodeBase*>& node() { return m_node; }
#endif // #ifdef NOTF_TEST
};

// ===================================================================================================================//

#ifdef NOTF_TEST

template<>
class PropertyGraph::Access<test::Harness> {
public:
    /// Freezes the PropertyGraph.
    /// All subsequent Property modifications and removals will create Delta objects until the Delta is resolved.
    /// Does nothing iff the render thread tries to freeze the graph multiple times.
    /// @param thread_id    ID of the freezing thread, is exposed for testability only.
    void freeze(const std::thread::id thread_id) { PropertyGraph::instance().freeze(thread_id); }

    /// Unfreezes the PropertyGraph and resolves all deltas.
    void unfreeze() { PropertyGraph::instance().unfreeze(PropertyGraph::instance().m_render_thread); }

    /// Number of nodes in the graph.
    size_t size() const { return PropertyGraph::instance().m_nodes.size(); }

    /// The value of a given Property, as read from a given thread.
    /// @param property     Property whose value to read.
    /// @param thread_id    (Pretend) Id of the calling thread.
    template<typename T>
    const T& read_property(Property<T>& property, const std::thread::id thread_id)
    {
        using Node = PropertyGraph::Node<T>;
        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);
            valid_ptr<Node*> node = static_cast<Node*>(graph.read_node(property.node(), thread_id).get());
            return node->value();
        }
    }

    /// Deletes a Property as if it was deleted from a given thread.
    /// @param property     Property whose value to read.
    /// @param thread_id    (Pretend) Id of the calling thread.
    void delete_property(const valid_ptr<PropertyGraph::NodeBase*> node, const std::thread::id thread_id)
    {
        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<RecursiveMutex> lock(graph.m_mutex);
            graph.delete_node(node, thread_id);
        }
    }
};

#else
template<>
class PropertyGraph::Access<test::Harness> {};
#endif // #ifdef NOTF_TEST

NOTF_CLOSE_NAMESPACE
