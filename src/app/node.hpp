#pragma once

#include <set>

#include "app/node_property.hpp"
#include "app/path.hpp"
#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Node;
} // namespace access

// ================================================================================================================== //

class Node : public receive_signals, public std::enable_shared_from_this<Node> {

    friend class access::_Node<Scene>;
    friend class access::_Node<NodeProperty>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Node<T>;

    /// Thrown when a node did not have the expected position in the hierarchy.
    using hierarchy_error = Scene::hierarchy_error;

    /// Validator function for a Property of type T.
    template<class T>
    using Validator = PropertyGraph::Validator<T>;

    /// Map to store Properties by their name.
    using PropertyMap = NodeProperty::PropertyMap;

    /// Exception thrown when the Node has gone out of scope (Scene must have been deleted).
    NOTF_EXCEPTION_TYPE(no_node_error);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(node_finalized_error);

protected:
    /// Factory token object to make sure that Node instances can only be created by correct factory methods.
    class FactoryToken {
        friend class Node;
        friend class RootNode;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(Node);

    /// Constructor.
    /// @param token    Factory token provided by Node::_add_child.
    /// @param scene    Scene to manage the node.
    /// @param parent   Parent of this node.
    Node(const FactoryToken&, Scene& scene, valid_ptr<Node*> parent);

    /// Destructor.
    virtual ~Node();

    /// @{
    /// The Scene containing this node.
    Scene& scene() { return m_scene; }
    const Scene& scene() const { return m_scene; }
    /// @}

    /// The SceneGraph containing this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph() const { return m_scene.graph(); }

    /// @{
    /// The parent of this node.
    valid_ptr<Node*> parent() { return m_parent; }
    valid_ptr<const Node*> parent() const { return m_parent; }
    /// @}

    /// The sibling-unique name of this node.
    const std::string& name() const { return m_name->value(); }

    /// Updates the name of this Node.
    /// @param name     Proposed new name for this node.
    /// @returns        Actual (sibling-unique) new name of this Node.
    const std::string& set_name(const std::string& name);

    /// Registers this Node as being dirty.
    /// A ScenGraph containing at least one dirty Node causes the Window to be redrawn at the next opportunity.
    /// Is virtual, so subclasses may decide to ignore this method, for example if the node is invisible.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    virtual void redraw() { SceneGraph::Access<Node>::register_dirty(*graph(), this); }

    // properties -------------------------------------------------------------

    /// Quick check whether this Node contains a Property of any type by the given name.
    /// @param name     Name of the Property to search for.
    /// @returns        True iff this Node has a Property with the given name.
    bool has_property(const std::string& name) const { return m_properties.count(name) != 0; }

    /// Queries a Property by name and type.
    /// @param name     Name of the requested Property.
    /// @returns        Requested Property, if one exists that matches both name and type.
    template<class T>
    risky_ptr<TypedNodeProperty<T>*> property(const std::string& name) const
    {
        auto it = m_properties.find(name);
        if (it == m_properties.end()) {
            return nullptr;
        }
        return dynamic_cast<TypedNodeProperty<T>*>(it->second.get());
    }

    // TODO: Node::operator[] for properties

    // hierarchy --------------------------------------------------------------

    /// Checks if this Node has a child Node with a given name.
    /// @param name     Name of the requested Node.
    bool has_child(const std::string& name)
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
        return _read_children().contains(name);
    }

    /// Returns a handle to a child Node with the given name.
    /// If you want to be sure that a child (of any type) by the name exists, call `has_child`.
    /// @param name     Name of the requested child node.
    /// @throws no_node_error    If there is no child with the given name, or it is of the wrong type.
    template<class T>
    NodeHandle<T> child(const std::string& name)
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
        return NodeHandle<T>(_read_children().get(name));
    }

    // TODO: methods to get child in the front / back / nth

    /// The number of direct children of this Node.
    size_t count_children() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
        return _read_children().size();
    }

    /// The number of all (direct and indirect) descendants of this Node.
    size_t count_descendants() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
        size_t result = 0;
        _count_descendants_impl(result);
        return result;
    }

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor         Potential ancestor to verify.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    bool has_ancestor(const valid_ptr<const Node*> ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Nodes.
    /// The root node is always a common ancestor, iff the two nodes are in the same scene.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws hierarchy_error If the two nodes are not in the same scene.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    valid_ptr<Node*> common_ancestor(valid_ptr<Node*> other);
    valid_ptr<const Node*> common_ancestor(valid_ptr<const Node*> other) const
    {
        return const_cast<Node*>(this)->common_ancestor(const_cast<Node*>(other.get()));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    template<class T>
    risky_ptr<const T*> first_ancestor() const // TODO: this should return a Handle, right?
    {
        if (!std::is_base_of<Node, T>::value) {
            return nullptr;
        }

        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*m_scene.graph()));

        valid_ptr<const Node*> next = parent();
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
        return const_cast<const Node*>(this)->first_ancestor<T>();
    }
    ///@}

    // z-order ----------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    bool is_in_front_of(const valid_ptr<Node*> sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    bool is_behind(const valid_ptr<Node*> sibling) const;

    /// Moves this Node in front of all of its siblings.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void stack_before(const valid_ptr<Node*> sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void stack_behind(const valid_ptr<Node*> sibling);

protected:
    /// Recursive implementation of `count_descendants`.
    void _count_descendants_impl(size_t& result) const
    {
        NOTF_ASSERT(SceneGraph::Access<Node>::mutex(*graph()).is_locked_by_this_thread());
        result += _read_children().size();
        for (const auto& child : _read_children()) {
            child->_count_descendants_impl(result);
        }
    }

    /// All children of this node, orded from back to front.
    /// Never creates a delta.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    const NodeContainer& _read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a new delta if the scene is frozen.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    NodeContainer& _write_children();

    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    ///             Note that all arguments for the Node base class are supplied automatically by this method.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle<T> _add_child(Args&&... args)
    {
        // create the node
        auto child = std::make_shared<T>(FactoryToken(), m_scene, this, std::forward<Args>(args)...);
        child->_finalize();
        auto handle = NodeHandle<T>::Access::template create<T>(child);

        { // store the node as a child
            NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
            _write_children().add(std::move(child));
        }

        return handle;
    }

    template<class T>
    void _remove_child(const NodeHandle<T>& handle)
    {
        if (!handle.is_valid()) {
            return;
        }
        std::shared_ptr<T> node = std::static_pointer_cast<T>(NodeHandle<T>::Access::get(handle).get());
        NOTF_ASSERT(node);

        { // remove the node from the child container
            NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
            NodeContainer& children = _write_children();
            children.erase(node);
        }
    }

    /// Removes all children of this node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void _clear_children();

    /// Constructs a new Property on this Node.
    /// @param name     Name of the Property.
    /// @param value    Initial value of the Property (also determines its type unless specified explicitly).
    /// @throws node_finalized_error
    ///                 If you call this method from anywhere but the constructor.
    /// @throws Path::not_unique_error
    ///                 If there already exists a Property of the same name on this Node.
    /// @throws no_graph_error
    ///                 If the SceneGraph of the node has been deleted.
    /// @throws NodeProperty::initial_value_error
    ///                 If the value of the Property could not be validated.
    template<class T>
    valid_ptr<TypedNodeProperty<T>*>
    _create_property(std::string name, T&& value, Validator<T> validator = {}, const bool has_body = true)
    {
        if (_is_finalized()) {
            notf_throw_format(node_finalized_error,
                              "Cannot create Property \"{}\" (or any new Property) on Node \"{}\", "
                              "or in fact any Node that has already been finalized",
                              name, this->name());
        }
        if (m_properties.count(name)) {
            notf_throw_format(Path::not_unique_error, "Node \"{}\" already has a Property named \"{}\"", this->name(),
                              name);
        }

        if (validator && !validator(value)) {
            notf_throw_format(NodeProperty::initial_value_error,
                              "Cannot create Property \"{}\" with value \"{}\", "
                              "that did not validate against the supplied Validator function",
                              name, value);
        }

        // create the property
        TypedNodePropertyPtr<T> property
            = NodeProperty::Access<Node>::create(std::forward<T>(value), this, std::move(validator), has_body);
        TypedNodeProperty<T>* result = property.get();
        auto it = m_properties.emplace(std::make_pair(std::move(name), std::move(property)));
        NOTF_ASSERT(it.second);
        NodeProperty::Access<Node>::set_name_iterator(*result, std::move(it.first));

        return result;
    }

private:
    /// Registers this node as "unfinalized" and creates the "name" property in the constructor.
    valid_ptr<TypedNodeProperty<std::string>*> _create_name();

    /// Finalizes this Node.
    void _finalize() const { s_unfinalized_nodes.erase(this); }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return s_unfinalized_nodes.count(this) == 0; }

    /// Registers this Node as being "tweaked".
    /// A Node is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    void _register_as_tweaked() { Scene::Access<Node>::register_tweaked(m_scene, shared_from_this()); }

    /// Cleans a tweaked Node when its SceneGraph is being unfrozen.
    /// @see _register_as_tweaked
    void _clean_tweaks();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The scene containing this node.
    Scene& m_scene;

    /// The parent of this node, is guaranteed to outlive this Node.
    valid_ptr<Node*> const m_parent;

    /// All children of this node, ordered from back to front and accessible by their (node-unique) name.
    NodeContainer m_children;

    /// All properties of this node, accessible by their (node-unique) name.
    PropertyMap m_properties;

    /// The parent-unique name of this Node.
    valid_ptr<TypedNodeProperty<std::string>*> const m_name;

    /// Only unfinalized nodes can create properties and creating new children in an unfinalized node creates no deltas.
    static thread_local std::set<valid_ptr<const Node*>> s_unfinalized_nodes;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Node<Scene> {
    friend class notf::Scene;

    /// All children of this node, ordered from back to front.
    /// It is okay to access the child container directly here, because the function is only used by the Scene to create
    /// a new delta NodeContainer.
    /// @param node     Node to operate on.
    static const NodeContainer& children(const Node& node) { return node.m_children; }

    /// Cleans a tweaked Node when its SceneGraph is being unfrozen.
    static void clean_tweaks(Node& node) { node._clean_tweaks(); }
};

template<>
class access::_Node<NodeProperty> {
    friend class notf::NodeProperty;

    /// Registers this Node as being "tweaked".
    /// A Node is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    static void register_tweaked(Node& node) { node._register_as_tweaked(); }
};

NOTF_CLOSE_NAMESPACE
