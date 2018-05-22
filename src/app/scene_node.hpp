#pragma once

#include "app/scene.hpp"
#include "app/scene_property.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE
namespace access { // forwards
template<class>
class _SceneNode;
} // namespace access

//====================================================================================================================//

class SceneNode : public receive_signals {
    friend class access::_SceneNode<Scene>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class T>
    using Access = access::_SceneNode<T>;

    /// Container used to store the children of a SceneNode.
    using NodeContainer = Scene::Access<SceneNode>::NodeContainer;

    /// Thrown when a node did not have the expected position in the hierarchy.
    using hierarchy_error = Scene::hierarchy_error;

    /// Exception thrown when the SceneNode has gone out of scope (Scene must have been deleted).
    NOTF_EXCEPTION_TYPE(no_node);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(node_finalized);

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `_add_child`.
    class FactoryToken {
        friend class SceneNode;
        friend class access::_SceneNode<Scene>;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this Node changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(SceneNode);

    /// Constructor.
    /// @param token    Factory token provided by Node::_add_child.
    /// @param scene    Scene to manage the node.
    /// @param parent   Parent of this node.
    SceneNode(const FactoryToken&, Scene& scene, valid_ptr<SceneNode*> parent);

    /// Destructor.
    virtual ~SceneNode();

    /// @{
    /// The Scene containing this node.
    Scene& scene() { return m_scene; }
    const Scene& scene() const { return m_scene; }
    /// @}

    /// The SceneGraph containing this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph() const { return m_scene.graph(); }

    /// @{
    /// The parent of this node.
    valid_ptr<SceneNode*> parent() { return m_parent; }
    valid_ptr<const SceneNode*> parent() const { return m_parent; }
    /// @}

    /// The user-defined name of this node, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

    /// Updates the name of this Node.
    /// @param name New name of this Node.
    const std::string& set_name(std::string name);

    /// Registers this Node as being dirty.
    /// A ScenGraph containing at least one dirty SceneNode causes the Window to be redrawn at the next opportunity.
    /// Is virtual, so subclasses may decide to ignore this method, for example if the node is invisible.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    virtual void redraw() { SceneGraph::Access<SceneNode>::register_dirty(*graph(), this); }

    // hierarchy --------------------------------------------------------------

    /// The number of direct children of this SceneNode.
    size_t count_children() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
        return _read_children().size();
    }

    /// The number of all (direct and indirect) descendants of this SceneNode.
    size_t count_descendants() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
        size_t result = 0;
        _count_descendants_impl(result);
        return result;
    }

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor     Potential ancestor to verify.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    bool has_ancestor(const valid_ptr<const SceneNode*> ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Nodes.
    /// The root node is always a common ancestor, iff the two nodes are in the same scene.
    /// @param other            Other SceneNode to find the common ancestor for.
    /// @throws hierarchy_error If the two nodes are not in the same scene.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    valid_ptr<SceneNode*> common_ancestor(valid_ptr<SceneNode*> other);
    valid_ptr<const SceneNode*> common_ancestor(valid_ptr<const SceneNode*> other) const
    {
        return const_cast<SceneNode*>(this)->common_ancestor(const_cast<SceneNode*>(other.get()));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T>
    risky_ptr<const T*> first_ancestor() const // TODO: this should return a Handle, right?
    {
        if (!std::is_base_of<SceneNode, T>::value) {
            return nullptr;
        }

        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*m_scene.graph()));

        valid_ptr<const SceneNode*> next = parent();
        while (next != next->parent()) {
            if (const T* result = dynamic_cast<T*>(next)) {
                return result;
            }
            next = next->parent();
        }
        return nullptr;
    }
    template<class T>
    risky_ptr<T*> first_ancestor()
    {
        return const_cast<const SceneNode*>(this)->first_ancestor<T>();
    }
    ///@}

    // z-order ------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_front_of(const valid_ptr<SceneNode*> sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_behind(const valid_ptr<SceneNode*> sibling) const;

    /// Moves this Node in front of all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_before(const valid_ptr<SceneNode*> sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_behind(const valid_ptr<SceneNode*> sibling);

protected:
    /// Recursive implementation of `count_descendants`.
    void _count_descendants_impl(size_t& result) const
    {
        NOTF_ASSERT(SceneGraph::Access<SceneNode>::mutex(*graph()).is_locked_by_this_thread());
        result += _read_children().size();
        for (const auto& child : _read_children()) {
            child->_count_descendants_impl(result);
        }
    }

    /// All children of this node, orded from back to front.
    /// Never creates a delta.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    const NodeContainer& _read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a new delta if the scene is frozen.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    NodeContainer& _write_children();

    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    ///             Note that all arguments for the Node base class are supplied automatically by this method.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<SceneNode, T>::value>>
    SceneNodeHandle<T> _add_child(Args&&... args)
    {
        // create the node
        auto child = std::make_shared<T>(FactoryToken(), m_scene, this, std::forward<Args>(args)...);
        child->_finalize();
        auto handle = SceneNodeHandle<T>(child);

        { // store the node as a child
            NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
            _write_children().add(std::move(child));
        }

        return handle;
    }

    template<class T>
    void _remove_child(const SceneNodeHandle<T>& handle)
    {
        if (!handle.is_valid()) {
            return;
        }
        std::shared_ptr<T> node = std::static_pointer_cast<T>(SceneNodeHandle<T>::Access::get(handle).get());
        NOTF_ASSERT(node);

        { // remove the node from the child container
            NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
            NodeContainer& children = _write_children();
            children.erase(node);
        }
    }

    /// Removes all children of this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    void _clear_children();

    /// Constructs a new Property on this SceneNode.
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @throws node_finalized      If you call this method from anywhere but the constructor.
    /// @throws Path::not_unique    If there already exists a Property of the same name on this SceneNode.
    /// @throws no_graph            If the SceneGraph of the node has been deleted.
    //    template<class T>
    //    PropertyHandler<T>& _create_property(std::string name, T&& value)
    //    {
    //        if (_is_finalized()) {
    //            notf_throw_format(node_finalized,
    //                              "Cannot create Property \"{}\" (or any new Property) on SceneNode \"{}\", "
    //                              "or in fact any SceneNode that has already been finalized",
    //                              name, this->name());
    //        }
    //        if (m_properties.count(name)) {
    //            notf_throw_format(Path::not_unique, "SceneNode \"{}\" already has a Property named \"{}\"",
    //            this->name(),
    //                              name);
    //        }

    //        // create the property
    //        PropertyHandler<T> result
    //            = PropertyGraph::Access<SceneNode>::create_property(graph()->property_graph(), std::move(value),
    //            this);
    //        auto it = m_properties.emplace(std::make_pair(std::move(name), result.property()));
    //        NOTF_ASSERT(it.second);

    //        return result;
    //    }

private:
    /// Finalizes this SceneNode.
    void _finalize() const { s_unfinalized_nodes.erase(this); }

    /// Whether or not this SceneNode has been finalized or not.
    bool _is_finalized() const { return s_unfinalized_nodes.count(this) == 0; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The scene containing this node.
    Scene& m_scene;

    /// The parent of this node.
    /// Is guaranteed to outlive this node.
    valid_ptr<SceneNode*> const m_parent;

    /// All children of this node, ordered from back to front.
    NodeContainer m_children;

    /// All properties of this node, accessible by their (node-unique) name.
    std::unordered_map<std::string, std::unique_ptr<PropertyHeadBase>> m_properties;

    /// The parent-unique name of this Node.
    std::string m_name;

    /// Only unfinalized nodes can create properties and creating new children in an unfinalized node creates no deltas.
    static thread_local std::set<valid_ptr<const SceneNode*>> s_unfinalized_nodes;
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_SceneNode<Scene> {
    friend class notf::Scene;

    /// Creates a factory Token so the Scene can create its RootNode.
    static SceneNode::FactoryToken create_token() { return notf::SceneNode::FactoryToken{}; }

    /// All children of this node, ordered from back to front.
    /// It is okay to access the child container directly here, because the function is only used by the Scene to create
    /// a new delta NodeContainer.
    /// @param node     SceneNode to operate on.
    static const SceneNode::NodeContainer& children(const SceneNode& node) { return node.m_children; }
};

// ===================================================================================================================//

/// The singular root node of a Scene hierarchy.
class RootSceneNode : public SceneNode {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param token    Factory token provided by the Scene.
    /// @param scene    Scene to manage the node.
    RootSceneNode(const FactoryToken& token, Scene& scene) : SceneNode(token, scene, this) {}

    /// Destructor.
    virtual ~RootSceneNode();

    /// Sets  a new child at the top of the Scene hierarchy (below the root).
    /// @param args Arguments that are forwarded to the constructor of the child.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T, class... Args>
    SceneNodeHandle<T> set_child(Args&&... args)
    {
        _clear_children();
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Removes the child of the root node, effectively clearing the Scene.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    void clear() { _clear_children(); }
};

NOTF_CLOSE_NAMESPACE
