#pragma once

#include "app/application.hpp"
#include "app/io/time.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thread-safe container of arbitrary values that can be connected using expressions.
///
/// Property
/// ========
///
/// A `Property` in NoTF is a container class for a single unit of *self-contained* data. An integer would be a prime
/// example, so would be a boolean. A node in a linked list would not qualify as Property, because it would need to link
/// to other Nodes and would therefore not be self-contained. Strings on the other hand do qualify, because even though
/// they contain a pointer to the heap, they are conceptually self-contained. Note that NoTF cannot enforce that each
/// Property type is indeed *self-contained*, use your own judgement.
///
/// Properties are used everywhere. The application-wide `time` and `framenumber` are Properties, so are widget
/// transformations, progress bar percentages or whatever else you need. By itself, a Property works just the same as a
/// normal value -- you can create, read, update and delete it without a need to consider what is going on behind the
/// scenes.
///
/// There are two reason for why Properties are so prevalent:
///
/// 1. They can be used to build *Property Expressions* in the `PropertyGraph`.
/// 2. They can safely be used from multiple threads and can be used to *freeze* the global state during rendering
/// without putting any responsibility on the user to do so.
///
/// The Property Graph
/// ==================
///
/// The Property Graph is the central management instance for all Properties in NoTF.
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
///    read/write | | | |                      |
///               v v v v                      |
///           +-------------+                  |
///           |    Delta    |                  |
///           +------+------+                  |
///                  | commit                  |
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
/// Drawbacks
/// =========
///
/// Of course, Properties are not perfect. Properties take up a large amount of space, especially when compared with
/// raw variables of particularly small types like booleans. Also, all Property values are stored on the heap, meaning
/// that even in the best case, all uses of a Property require at least one heap access. Reading a Property might block
/// while its expression is evaluated, and since expressions are defined by the user, the block duration is undetermined
/// (if you put an infinite loop into an expression, you can wait forever ... go figure). Lastly, if the PropertyGraph
/// is currently frozen, modifying a Property will create a new Delta object on the heap.
///
/// As always, use your own judgement. If you have a value that is not used for rendering, that is *const* or can only
/// be accessed by a single thread at a time and that will never be part or target of an expression, there is nothing
/// stopping you from just using a plain old variable instead.
///
class PropertyGraph {

    friend class detail::PropertyBase;

    template<typename>
    friend class Property;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error)

private:
    /// Error thrown when a Property that is referenced as a dependency has been deleted.
    NOTF_EXCEPTION_TYPE(property_deleted_error)

    //=========================================================================

    template<typename>
    class NodeDelta;

    /// Untyped base type for all PropertyNode types.
    /// It is required that you hold the PropertyGraph mutex for all operations on PropertyNodes.
    class NodeBase {

        // methods ------------------------------------------------------------
    public:
        /// Virtual destructor.
        virtual ~NodeBase();

        /// Creates a properly typed copy of this PropertyNode instance.
        virtual std::unique_ptr<NodeBase> copy_to_delta() = 0;

        /// Id of this PropertyNode. Is simply `this` unless this Node is part of a Delta.
        virtual NodeBase* id() { return this; }

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
        /// @throws property_deleted_error  If any of the dependencies was deleted.
        void _register_with_dependencies()
        {
            NodeBase* const my_id = id();
            for (NodeBase* dependency_id : m_dependencies) {
                risky_ptr<NodeBase> dependency = PropertyGraph::instance().write_node(dependency_id);
                if (!dependency) {
                    notf_throw(property_deleted_error, "Dependency was deleted");
                }
                NOTF_ASSERT(std::find(dependency->m_affected.begin(), dependency->m_affected.end(), my_id)
                            == dependency->m_affected.end());
                dependency->m_affected.emplace_back(my_id);
            }
        }

        /// Removes all dependencies from this property.
        void _unregister_from_dependencies()
        {
            NodeBase* const my_id = id();
            for (NodeBase* dependency_id : m_dependencies) {
                risky_ptr<NodeBase> dependency = PropertyGraph::instance().write_node(dependency_id);
                if (!dependency) {
                    continue; // ignore
                }
                auto it = std::find(dependency->m_affected.begin(), dependency->m_affected.end(), my_id);
                NOTF_ASSERT(it != dependency->m_affected.end());
                *it = std::move(dependency->m_affected.back());
                dependency->m_affected.pop_back();
            }
            m_dependencies.clear();
        }

        /// Set all affected properties dirty.
        void _set_affected_dirty()
        {
            for (NodeBase* affected_id : m_affected) {
                if (risky_ptr<NodeBase> affected = PropertyGraph::instance().write_node(affected_id)) {
                    affected->_set_dirty();
                }
            }
        }

        /// Detects potential cyclic dependencies in the graph before they are introduced.
        /// @param dependencies                 All PropertyNodes that this node will potentially be dependent on.
        /// @throws no_dag_error     If the expression would introduce a cyclic dependency into the graph.
        void _detect_cycles(const std::vector<NodeBase*>& dependencies);

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        std::vector<NodeBase*> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<NodeBase*> m_affected;

        /// Time when this Property was last defined.
        /// Is invalid if the Property is dirty (its expression needs to be evaluated).
        Time m_time = Time::now();
    };

    //=========================================================================

    /// A typed PropertyNode in the Property graph that manages all of the Properties connection, its value and optional
    /// expression.
    template<typename T, typename = std::enable_if_t<std::is_copy_assignable<T>::value>>
    class Node : public NodeBase {

        template<typename>
        friend class PropertyNodeDelta;

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        Node(T value) : NodeBase(), m_value(std::move(value)) {}

        /// Creates a properly typed copy of this PropertyNode instance.
        virtual std::unique_ptr<NodeBase> copy_to_delta() override { return std::make_unique<NodeDelta<T>>(*this); }

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
        void set_expression(std::function<T(const PropertyGraph&)> expression, std::vector<NodeBase*> dependencies)
        {
            if (expression == m_expression) {
                return;
            }
            if (!expression) {
                return _ground();
            }
            _detect_cycles(dependencies);

            _unregister_from_dependencies();
            m_expression = std::move(expression);
            m_dependencies = std::move(dependencies);
            try {
                _register_with_dependencies();
            }
            catch (const property_deleted_error&) {
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
            m_value = m_expression(PropertyGraph::instance());
            _set_clean();
        }

        // fields -------------------------------------------------------------
    protected:
        /// Expression evaluating to a new value for this Property.
        std::function<T(const PropertyGraph&)> m_expression;

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

        /// Id of the copied PropertyNode.
        virtual NodeBase* id() { return m_id; }

        // fields -------------------------------------------------------------
    private:
        /// Id of the copied PropertyNode.
        NodeBase* const m_id;
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
        virtual NodeBase* node() const = 0;

        /// Returns true iff this Delta is a deletion Delta, false if it is a modification Delta.
        bool is_deletion() const { return !is_modification(); }
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
        virtual NodeBase* node() const override { return m_node.get(); }

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
        virtual NodeBase* node() const override { return m_node; }

    private:
        /// Property that was deleted.
        NodeBase* m_node;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph singleton.
    /// @throws application_initialization_error    If you call this method before an Application has been initialized.
    static PropertyGraph& instance() { return Application::instance().property_graph(); }

    /// Returns a PropertyNode for reading without creating a new delta.
    /// @returns The requested Node or nullptr, if the node has a deletion delta.
    risky_ptr<NodeBase> read_node(NodeBase* ptr)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen() || std::this_thread::get_id() == m_reader_thread) {
            return ptr; // direct access
        }

        // return the frozen node if there is no delta for it
        auto it = m_delta.find(ptr);
        if (it == m_delta.end()) {
            return ptr;
        }

        // return an existing modification delta
        const auto& delta = it->second;
        if (delta->is_modification()) {
            return delta->node();
        }
        return nullptr;
    }

    /// Returns a PropertyNode for writing.
    /// Creates a new NodeDelta if the graph is frozen.
    /// @returns The requested Node or nullptr, if the node already has a deletion delta.
    risky_ptr<NodeBase> write_node(NodeBase* ptr)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen()) {
            return ptr;
        }

        // return an existing modification delta
        auto it = m_delta.find(ptr);
        if (it != m_delta.end()) {
            if (it->second->is_modification()) {
                return it->second->node();
            }
            return nullptr;
        }

        // create a new modification delta
        auto new_delta = std::make_unique<ModificationDelta>(ptr->copy_to_delta());
        auto result = new_delta->node();
        m_delta.emplace(std::make_pair(ptr, std::move(new_delta)));
        return result;
    }

    /// Checks if the PropertyGraph is currently frozen or not.
    bool is_frozen() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_reader_thread != std::thread::id();
    }

    /// Freezes the PropertyGraph.
    /// All subsequent Property modifications and removals will create Delta objects until the Delta is resolved.
    /// Does nothing iff the reader thread tries to freeze the graph multiple times.
    /// @throws thread_error    If any but the reading thread tries to freeze the graph.
    void freeze();

    /// Unfreezes the PropertyGraph and resolves all deltas.
    /// @throws thread_error    If any but the reading thread tries to unfreeze the graph.
    void unfreeze();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The current delta.
    robin_map<const NodeBase*, std::unique_ptr<DeltaBase>> m_delta;

    /// Mutex guarding the graph.
    mutable Mutex m_mutex;

    /// Flag whether the graph currently has a Delta or not.
    std::thread::id m_reader_thread{0};
};

// ===================================================================================================================//

namespace detail {

/// Base class of all Properties.
/// Necessary so you can collect Properties of various types together in a vector of dependencies for `set_expression`.
class PropertyBase {
    template<typename T>
    friend class notf::Property;

    using NodeBase = PropertyGraph::NodeBase;

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// Unique pointer to the node of this Property in the graph.
    std::unique_ptr<PropertyGraph::NodeBase> m_node = nullptr;
};

} // namespace detail

// ===================================================================================================================//

/// An object managing a single Property in the PropertyGraph.
/// Contains a unique_ptr to a PropertyNode in the graph.
template<typename T>
class Property : public detail::PropertyBase {

    using Node = PropertyGraph::Node<T>;
    using NodeBase = PropertyGraph::NodeBase;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Property)

    /// Constructor.
    /// @param value    Initial value of the Property.
    explicit Property(T&& value) : PropertyBase() { m_node = std::make_unique<Node>(std::forward<T>(value)); }

    /// Move Constructor.
    /// @param other    Property to move from.
    Property(Property&& other) { m_node.swap(other.m_node); }

    /// The value of this Property.
    const T& value()
    {
        NOTF_ASSERT_MSG(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<std::mutex> lock(graph.m_mutex);
            Node* node = dynamic_cast<Node>(make_raw(graph.read_node(m_node.get())));
            NOTF_ASSERT(node);
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
            std::lock_guard<std::mutex> lock(graph.m_mutex);
            Node* node = dynamic_cast<Node>(make_raw(graph.write_node(m_node.get())));
            NOTF_ASSERT(node);
            node->set_value(std::forward<T>(value));
        }
    }

    /// Sets an expression of this Property.
    /// @param expression       New expression defining the value of the Property.
    /// @param dependency       All Property dependencies. It is of critical importance that all Properties in the
    ///                         expression are listed.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(std::function<T(const PropertyGraph&)>&& expression,
                        const std::vector<std::reference_wrapper<detail::PropertyBase>>& dependency)
    {
        NOTF_ASSERT_MSG(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        // collect unique pointers from all passed property dependencies
        std::vector<NodeBase*> dependencies;
        dependencies.reserve(dependencies.size());
        for (const auto& ref : dependency) {
            const NodeBase* ptr = ref.get().m_node.get();
            if (std::find(dependencies.begin(), dependencies.end(), ptr) == dependencies.end()) {
                dependencies.emplace_back(ptr);
            }
        }

        PropertyGraph& graph = PropertyGraph::instance();
        {
            std::lock_guard<std::mutex> lock(graph.m_mutex);
            Node* node = dynamic_cast<Node>(make_raw(graph.write_node(m_node.get())));
            NOTF_ASSERT(node);
            node->set_expression(std::move(expression), std::move(dependencies));
        }
    }
};

NOTF_CLOSE_NAMESPACE
