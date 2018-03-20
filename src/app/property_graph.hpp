#pragma once

#include <mutex>
#include <thread>

#include "app/application.hpp"
#include "app/io/time.hpp"
#include "common/map.hpp"
#include "common/optional.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thread-safe container of arbitrary values that can be connected using expressions.
///
/// Multithreading and Deltas
/// =========================
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
class PropertyGraph {

    friend class detail::PropertyBase;

    template<typename>
    friend class Property;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error)

    //=========================================================================
private:
    /// Untyped base type for all PropertyNode types.
    class PropertyNodeBase {

        // methods ------------------------------------------------------------
    public:
        /// Virtual destructor.
        virtual ~PropertyNodeBase();

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

        /// Freezing a property means removing its expression without changing its value.
        void _freeze()
        {
            _clear_expression();
            _unregister_from_dependencies();
            _set_clean();
        }

        /// Registers this property as affected with all of its dependencies.
        void _register_with_dependencies()
        {
            for (PropertyNodeBase* dependency : m_dependencies) {
                NOTF_ASSERT(std::find(dependency->m_affected.begin(), dependency->m_affected.end(), this)
                                == dependency->m_affected.end(),
                            "Duplicate insertion of Property \"" << this << "\" as affected of dependency: \""
                                                                 << dependency << "\"");
                dependency->m_affected.emplace_back(this);
            }
        }

        /// Removes all dependencies from this property.
        void _unregister_from_dependencies()
        {
            for (PropertyNodeBase* dependency : m_dependencies) {
                auto it = std::find(dependency->m_affected.begin(), dependency->m_affected.end(), this);
                NOTF_ASSERT(it != dependency->m_affected.end(),
                            "Failed to unregister previously registered affected Property \""
                                << this << "\" from dependency: \"" << dependency << "\"");
                *it = std::move(dependency->m_affected.back());
                dependency->m_affected.pop_back();
            }
            m_dependencies.clear();
        }

        /// Set all affected properties dirty.
        void _set_affected_dirty()
        {
            for (PropertyNodeBase* affected : m_affected) {
                affected->_set_dirty();
            }
        }

        /// Detects potential cyclic dependencies in the graph before they are introduced.
        /// @param dependencies                 All PropertyNodes that this node will potentially be dependent on.
        /// @throws no_dag_error     If the expression would introduce a cyclic dependency into the graph.
        void _detect_cycles(const std::vector<PropertyNodeBase*>& dependencies) const;

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        std::vector<PropertyNodeBase*> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<PropertyNodeBase*> m_affected;

        /// Time when this Property was last defined.
        /// Is invalid if the Property is dirty (its expression needs to be evaluated).
        Time m_time = Time::now();
    };

    //=========================================================================

    /// A typed PropertyNode in the Property graph that manages all of the Properties connection, its value and optional
    /// expression.
    template<typename T>
    class PropertyNode : public PropertyNodeBase {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        PropertyNode(T&& value) : m_value(std::move(value)) {}

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
                _freeze();
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
        void
        set_expression(std::function<T(const PropertyGraph&)> expression, std::vector<PropertyNodeBase*> dependencies)
        {
            if (expression == m_expression) {
                return;
            }
            if (!expression) {
                return _freeze();
            }
            _detect_cycles(dependencies);

            _unregister_from_dependencies();
            m_expression = std::move(expression);
            m_dependencies = std::move(dependencies);
            _register_with_dependencies();

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
            NOTF_ASSERT(m_expression, "Cannot evaluate expression from ground Property: \"" << this << "\"");
            m_value = m_expression(PropertyGraph::instance());
            _set_clean();
        }

        // fields -------------------------------------------------------------
    private:
        /// Value held by the Property.
        T m_value;

        /// Expression evaluating to a new value for this Property.
        std::function<T(const PropertyGraph&)> m_expression;
    };

    //=========================================================================

    struct PropertyDeltaBase {};

    template<typename T>
    struct PropertyDelta : public PropertyDeltaBase {
        /// Constructor.
        /// @param value    Value of the modified Property, leave empty for a deletion delta.
        PropertyDelta(std::optional<T>&& value = {}) : value(std::move(value)) {}

        /// Optional value, if modified.
        std::optional<T> value;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph singleton.
    /// @throws application_initialization_error    If you call this method before an Application has been initialized.
    static PropertyGraph& instance() { return Application::instance().property_graph(); }

    /// Tells the PropertyGraph to enable the Delta state.
    /// All subsequent Property modifications and removals will create Delta objects until the Delta is resolved.
    void enable_delta()
    {
        const auto thread_id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(m_mutex);
        if (thread_id == m_reader_thread) {
            return;
        }
        NOTF_ASSERT(thread_id != std::thread::id(0), "Unexpected second reading thread of a PropertyGraph");
        m_reader_thread = thread_id;
    }

    void resolve_delta();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The current delta.
    robin_map<const PropertyNodeBase*, std::unique_ptr<PropertyDeltaBase>> m_delta;

    /// Mutex guarding the graph.
    mutable std::mutex m_mutex;

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

    using NodeBase = PropertyGraph::PropertyNodeBase;

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// Unique pointer to the node of this Property in the graph.
    std::unique_ptr<PropertyGraph::PropertyNodeBase> m_node = nullptr;
};

} // namespace detail

// ===================================================================================================================//

/// An object managing a single Property in the PropertyGraph.
/// Contains a unique_ptr to a PropertyNode in the graph.
template<typename T>
class Property : public detail::PropertyBase {

    using Node = PropertyGraph::PropertyNode<T>;
    using DeltaBase = PropertyGraph::PropertyDeltaBase;
    using Delta = PropertyGraph::PropertyDelta<T>;

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
        NOTF_ASSERT(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");
        PropertyGraph& graph = PropertyGraph::instance();
        const auto thread_id = std::this_thread::get_id();
        {
            std::lock_guard<std::mutex> lock(graph.m_mutex);
            if (graph.m_reader_thread == std::thread::id(0) || thread_id == graph.m_reader_thread) {
                return _node().value();
            }
            else {
                auto it = graph.m_delta.find(m_node.get());
                if (it == graph.m_delta.end()) {
                    return _node().value();
                }
                NOTF_ASSERT(dynamic_cast<Delta>(*it), "Encountered Property Delta of wrong type");
                const Delta* delta = static_cast<Delta>(*it);
                if (delta->value.has_value()) {
                    return delta->value.value();
                } else {
                    // TODO: throw Property deleted error
                    // TODO: CONTINUE HERE
                    // Is that even a possibility anymore, that you would access a PropertyNode that has been deleted?
                    // If it were deleted, you wouldn't have the Property anymore.
                }
            }
        }
    }

    /// Sets the value of this Property.
    /// @param value            New value.
    void set_value(T&& value)
    {
        NOTF_ASSERT(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");
        std::lock_guard<std::mutex> lock(PropertyGraph::instance().m_mutex);
        // TODO: implement delta check
        _node().set_value(std::forward<T>(value));
    }

    /// Sets an expression of this Property.
    /// @param expression       New expression defining the value of the Property.
    /// @param dependency       All Property dependencies. It is of critical importance that all Properties in the
    ///                         expression are listed.
    /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
    void set_expression(std::function<T(const PropertyGraph&)>&& expression,
                        const std::vector<std::reference_wrapper<detail::PropertyBase>>& dependency)
    {
        NOTF_ASSERT(m_node, "Access to invalid Property instance (are you using an object that was moved from?)");

        // collect unique pointers from all passed property dependencies
        std::vector<NodeBase*> dependencies;
        dependencies.reserve(dependencies.size());
        for (const auto& ref : dependency) {
            const NodeBase* ptr = ref.get().m_node.get();
            if (std::find(dependencies.begin(), dependencies.end(), ptr) == dependencies.end()) {
                dependencies.emplace_back(ptr);
            }
        }

        { // set the expression
            std::lock_guard<std::mutex> lock(PropertyGraph::instance().m_mutex);
            _node().set_expression(std::move(expression), std::move(dependencies));
        }
    }

private:
    /// Convenience access to the typed PropertyNode.
    Node& _node() { return *static_cast<Node*>(m_node.get()); }
};

NOTF_CLOSE_NAMESPACE
