#pragma once

#include <set>

#include "app/node_handle.hpp"
#include "app/path.hpp"
#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Node;
} // namespace access

// ================================================================================================================== //

class Node : public receive_signals, public std::enable_shared_from_this<Node> {

    friend class RootNode;
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
    NodeHandle<Node> parent() { return NodeHandle<Node>(raw_pointer(m_parent)); }
    const NodeHandle<Node> parent() const { return NodeHandle<Node>(raw_pointer(m_parent)); }
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

    /// Returns a Property of this Node by name and type.
    /// @param name     Name of the requested Property.
    /// @returns        Requested Property, if one exists that matches both name and type.
    template<class T>
    PropertyHandle<T> property(const std::string& name) const
    {
        auto it = m_properties.find(name);
        if (it == m_properties.end()) {
            return {}; // empty default
        }
        return PropertyHandle<T>(std::dynamic_pointer_cast<TypedNodePropertyPtr<T>>(it->second));
    }

    /// Returns a Property of this Node by path and type.
    /// @param path     Path uniquely identifying the requested Property.
    /// @returns        Requested Property, if one exists that matches both name and type.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    template<class T>
    PropertyHandle<T> property(const Path& path)
    {
        return PropertyHandle<T>(std::dynamic_pointer_cast<TypedNodePropertyPtr<T>>(_property(path)));
    }

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
    /// @returns        The requested child Node.
    /// @throws no_node_error    If there is no child with the given name, or it is of the wrong type.
    template<class T>
    NodeHandle<T> child(const std::string& name)
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*graph()));
        return NodeHandle<T>(_read_children().get(name));
    }

    /// Searches for and returns a descendant of this Node.
    /// @param path     Path uniquely identifying a child Node.
    /// @returns        Handle to the requested child Node.
    ///                 Is empty if the Node doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T>
    NodeHandle<T> node(const Path& path)
    {
        return NodeHandle<T>(std::dynamic_pointer_cast<T>(_node(path)));
    }

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

    /// Finds and returns the first common ancestor of two Nodes.
    /// The root node is always a common ancestor, iff the two nodes are in the same scene.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws hierarchy_error If the two nodes are not in the same scene.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    NodeHandle<Node> common_ancestor(valid_ptr<Node*> other);

    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @returns    Typed handle of the first ancestor with the requested type, can be empty if none was found.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    template<class T, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle<T> first_ancestor()
    {
        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(*m_scene.graph()));

        valid_ptr<const Node*> next = m_parent;
        while (next != next->m_parent) {
            if (const T* result = dynamic_cast<T*>(next)) {
                return NodeHandle<T>(result);
            }
            next = next->m_parent;
        }
        return {};
    }

    // TODO: methods to get child in the front / back / nth

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
    /// Private and untyped implementation of `property` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Property.
    /// @returns        The requested NodeProperty.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    NodePropertyPtr _property(const Path& path);

    /// Recursive path iteration method to query a related Property.
    /// @param path     Path uniquely identifying a Property.
    /// @param index    Index of the next child in the path
    /// @param result   [OUT] Result of the query.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    void _property(const Path& path, const uint index, NodePropertyPtr& result);

    /// Private and untyped implementation of `node` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        Handle to the requested Node. Is empty if the Node doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    NodePtr _node(const Path& path);

    /// Recursive path iteration method to query a descendant Node.
    /// @param path     Path uniquely identifying a Node.
    /// @param index    Index of the next child in the path
    /// @param result   [OUT] Result of the query.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    void _node(const Path& path, const uint index, NodePtr& result);

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
        auto handle = NodeHandle<T>(child);

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
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @param validator    Optional validator function.
    /// @param has_body     Whether or not the Property will have a Property body in the Property Graph.
    /// @throws node_finalized_error
    ///                 If you call this method from anywhere but the constructor.
    /// @throws Path::not_unique_error
    ///                 If there already exists a Property of the same name on this Node.
    /// @throws no_graph_error
    ///                 If the SceneGraph of the node has been deleted.
    /// @throws NodeProperty::initial_value_error
    ///                 If the value of the Property could not be validated.
    template<class T>
    PropertyHandle<T>
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
            = _create_property_impl(std::move(name), std::forward<T>(value), std::move(validator), has_body);
        return PropertyHandle<T>(std::move(property));
    }

private:
    /// Creates a new Property on the node, used by `_create_property` and `_create_name`.
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @param validator    Optional validator function.
    /// @param has_body     Whether or not the Property will have a Property body in the Property Graph.
    template<class T>
    TypedNodePropertyPtr<T>
    _create_property_impl(std::string name, T&& value, Validator<T> validator, const bool has_body)
    {
        TypedNodePropertyPtr<T> property
            = NodeProperty::Access<Node>::create(std::forward<T>(value), this, std::move(validator), has_body);
        auto it = m_properties.emplace(std::make_pair(std::move(name), property));
        NOTF_ASSERT(it.second);
        NodeProperty::Access<Node>::set_name_iterator(*property, std::move(it.first));
        return property;
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

    /// Only unfinalized nodes can create properties and creating new children in an unfinalized node creates no
    /// deltas.
    static thread_local std::set<valid_ptr<const Node*>> s_unfinalized_nodes;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Node<Scene> {
    friend class notf::Scene;

    /// All children of this node, ordered from back to front.
    /// It is okay to access the child container directly here, because the function is only used by the Scene to
    /// create a new delta NodeContainer.
    /// @param node     Node to operate on.
    static const NodeContainer& children(const Node& node) { return node.m_children; }

    /// Cleans a tweaked Node when its SceneGraph is being unfrozen.
    static void clean_tweaks(Node& node) { node._clean_tweaks(); }

    /// Recursive path iteration method to query a related Property.
    /// @param node     Node to operate on.
    /// @param path     Path uniquely identifying a Property.
    /// @param index    Index of the next child in the path
    /// @param returns  Requested Property, if one exists that matches both name and type.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    static NodePropertyPtr property(Node& node, const Path& path, const uint index)
    {
        NodePropertyPtr result;
        node._property(path, index, result);
        return result;
    }

    /// Recursive path iteration method to query a descendant Node.
    /// @param node     Node to operate on.
    /// @param path     Path uniquely identifying a Node.
    /// @param index    Index of the next child in the path
    /// @param returns  Requested Node, if one exists that matches both name and type.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    static NodePtr node(Node& node, const Path& path, const uint index)
    {
        NodePtr result;
        node._node(path, index, result);
        return result;
    }
};

template<>
class access::_Node<NodeProperty> {
    friend class notf::NodeProperty;

    /// Registers this Node as being "tweaked".
    /// A Node is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    static void register_tweaked(Node& node) { node._register_as_tweaked(); }
};

NOTF_CLOSE_NAMESPACE