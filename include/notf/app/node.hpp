#pragma once

#include <unordered_set>

#include "./property.hpp"

NOTF_OPEN_NAMESPACE

// node ============================================================================================================= //

class Node : public std::enable_shared_from_this<Node> {

    // type --------------------------------------------------------------------------------------------------------- //
public:
    /// Exception thrown by `get_property` if either the name of the type of the requested Property is wrong.
    NOTF_EXCEPTION_TYPE(NoSuchPropertyError);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(FinalizedError);

private:
    /// Owning list of child Nodes, ordered from back to front.
    using ChildList = std::vector<NodePtr>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Node(valid_ptr<Node*> parent);

public:
    NOTF_NO_COPY_OR_ASSIGN(Node);

    /// Destructor.
    virtual ~Node();

    /// Uuid of this Node.
    const Uuid& get_uuid() const { return m_uuid; }

    /// The Node-unique name of this Node.
    /// If this Node does not have a user-defined name, a random one is generated for it.
    std::string_view get_name() const;

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    std::string_view set_name(const std::string& name);

    /// Run time access to a Property of this Node.
    template<class T>
    PropertyHandle<T> get_property(std::string_view name) const
    {
        AnyPropertyPtr& property = _get_property(name);
        if (!property) {
            NOTF_THROW(NoSuchPropertyError, "Node \"{}\" has no Property called \"{}\"", get_name(), name);
        }

        PropertyPtr<T> typed_property = std::dynamic_pointer_cast<Property<T>>(property);
        if (!typed_property) {
            NOTF_THROW(NoSuchPropertyError,
                       "Property \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), property->get_type_name(), type_name<T>()());
        }
        return PropertyHandle(std::move(typed_property));
    }

    // hierarchy --------------------------------------------------------------

    /// The parent of this Node.
    NodeHandle get_parent() const;

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor         Potential ancestor to verify.
    bool has_ancestor(NodeHandle ancestor) const;

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// If the handle passed in is expired, the returned handle will also be expired.
    /// @param other    Other Node to find the common ancestor for.
    NodeHandle get_common_ancestor(NodeHandle other) const;

    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @returns    Typed handle of the first ancestor with the requested type, can be empty if none was found.
    template<class T, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle get_first_ancestor()
    {
        for (const Node* next = _get_parent(); next != next->_get_parent(); next = next->_get_parent()) {
            if (auto* result = dynamic_cast<T*>(next)) { return NodeHandle(result->weak_from_this()); }
        }
        return {};
    }

    /// The number of direct children of this Node.
    size_t count_children() const { return _read_children().size(); }

    /// Returns a handle to a child Node at the given index.
    /// Index 0 is the node furthest back, index `size() - 1` is the child drawn at the front.
    /// @param index    Index of the Node.
    /// @returns        The requested child Node.
    /// @throws out_of_bounds    If the index is out-of-bounds or the child Node is of the wrong type.
    NodeHandle get_child(size_t index) const;

    // z-order ----------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_before(const NodeHandle& sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_behind(const NodeHandle& sibling) const;

    /// Moves this Node in front of all of its siblings.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_before(const NodeHandle& sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_behind(const NodeHandle& sibling);

protected:
    /// Implementation specific query of a Property.
    virtual AnyPropertyPtr& _get_property(std::string_view name) const = 0;

    /// Calculates the combined hash value of all Properties.
    /// Note that this method does not store the new hash value, so it can be compared with the old one.
    virtual size_t _calculate_property_hash() const = 0;

    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle _create_child(Args&&... args)
    {
        auto child = std::make_shared<T>(this, std::forward<Args>(args)...);
        child->_finalize();

        auto handle = NodeHandle(child);
        _write_children().emplace_back(std::move(child));
        return handle;
    }

    /// @{
    /// Removes a child from this node.
    /// @param handle   Handle of the node to remove.
    void _remove_child(NodePtr child);
    void _remove_child(NodeHandle child) { _remove_child(child.m_node.lock()); }
    /// @}

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// @param other    Other Node to find the common ancestor for.
    const Node* _get_common_ancestor(const Node* other) const;

    /// Direct access to the parent of this Node.
    Node* _get_parent() const;

    /// @{
    /// Changes the parent of this Node by first adding it to the new parent and then removing it from its old one.
    /// @param new_parent   New parent of this Node. If it is the same as the old, this method does nothing.
    void _set_parent(NodePtr new_parent);
    void _set_parent(NodeHandle new_parent) { _set_parent(new_parent.m_node.lock()); }
    /// @}

    /// All children of this node, orded from back to front.
    /// Never modifies the cache.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const ChildList& _read_children(std::thread::id thread_id = std::this_thread::get_id()) const;

    /// All children of this node, orded from back to front.
    /// Will copy the current list of children into the cache, if the cache is empty and the scene is frozen.
    ChildList& _write_children();

    /// Updates the Node hash
    void _update_node_hash() const;

    /// Deletes the frozen child list copy, if one exists.
    void _clear_frozen() { delete m_cache.exchange(nullptr); }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return s_unfinalized_nodes.count(this) == 0; }

private:
    /// All children of the parent.
    const ChildList& _read_siblings() const;

    /// Finalizes this Node.
    /// Called on every new Node instance right after the Constructor of the most derived class has finished.
    void _finalize() const { s_unfinalized_nodes.erase(this); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Parent of this Node.
    Node* m_parent;

    /// Name of this Node. Is empty until requested for the first time or set using `set_name`.
    mutable std::string_view m_name;

    /// All children of this Node, ordered from back to front (later Nodes are drawn on top of earlier ones).
    ChildList m_children;

    /// Pointer to a frozen copy of the list of children, if it was modified while the Graph was frozen.
    std::atomic<ChildList*> m_cache{nullptr};

    /// Hash of all Property values of this Node.
    size_t m_property_hash;

    /// Combines the Property hash with the Node hashes of all children in order.
    size_t m_node_hash = 0;

    /// Used to distinguish "finalized" from "unfinalized" RunTime Nodes.
    /// Only unfinalized Nodes can create properties.
    /// Also, creating children in an unfinalized node creates no deltas.
    static thread_local std::unordered_set<const Node*> s_unfinalized_nodes;
};

NOTF_CLOSE_NAMESPACE
