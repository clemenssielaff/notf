#pragma once

#include "notf/common/bitset.hpp"
#include "notf/common/tuple.hpp"

#include "./property_handle.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

template<class T, class = void>
struct has_allowed_child_types : std::false_type {};
template<class T>
struct has_allowed_child_types<T, std::void_t<typename T::allowed_child_types>> : std::true_type {};

template<class T, class = void>
struct has_forbidden_child_types : std::false_type {};
template<class T>
struct has_forbidden_child_types<T, std::void_t<typename T::forbidden_child_types>> : std::true_type {};

template<class T, class = void>
struct has_allowed_parent_types : std::false_type {};
template<class T>
struct has_allowed_parent_types<T, std::void_t<typename T::allowed_parent_types>> : std::true_type {};

template<class T, class = void>
struct has_forbidden_parent_types : std::false_type {};
template<class T>
struct has_forbidden_parent_types<T, std::void_t<typename T::forbidden_parent_types>> : std::true_type {};

template<class A, class B>
constexpr bool can_node_parent()
{
    // both A and B must be derived from Node
    if constexpr (std::negation_v<std::conjunction<std::is_base_of<Node, A>, std::is_base_of<Node, B>>>) {
        return false;
    }

    // if A has a list of explicitly allowed child types, B must be in it
    if constexpr (has_allowed_child_types<A>::value) {
        if (!is_derived_from_one_of_tuple_v<B, typename A::allowed_child_types>) { return false; }
    }
    // ... otherwise, if A has a list of explicitly forbidden child types, B must NOT be in it
    else if constexpr (has_forbidden_child_types<A>::value) {
        if (is_derived_from_one_of_tuple_v<B, typename A::forbidden_child_types>) { return false; }
    }

    // if B has a list of explicitly allowed parent types, A must be in it
    if constexpr (has_allowed_parent_types<B>::value) {
        if (!is_derived_from_one_of_tuple_v<A, typename B::allowed_parent_types>) { return false; }
    }
    // ... otherwise, if B has a list of explicitly forbidden parent types, A must NOT be in it
    else if constexpr (has_forbidden_parent_types<B>::value) {
        if (is_derived_from_one_of_tuple_v<A, typename B::forbidden_parent_types>) { return false; }
    }

    return true;
}

} // namespace detail

// node ============================================================================================================= //

/// Base class for both RunTimeNode and CompileTimeNode.
class Node : public std::enable_shared_from_this<Node> {

    friend Accessor<Node, NodeMasterHandle>;
    friend Accessor<Node, AnyRootNode>;

    // type --------------------------------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Node);

    /// Exception thrown by `get_property` if either the name of the type of the requested Property is wrong.
    NOTF_EXCEPTION_TYPE(NoSuchPropertyError);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(FinalizedError);

private:
    /// Wrapper around a newly created Node.
    /// Can be cast to a NodeMasterHandle or NodeHandle.
    using NewNode = detail::NodeMasterHandleCastable;

    /// Owning list of child Nodes, ordered from back to front.
    using ChildList = std::vector<NodePtr>;

    // property observer ------------------------------------------------------

    /// Internal reactive function that is subscribed to all visible Properties and marks the Node as dirty, should one
    /// of them change.
    class PropertyObserver : public Subscriber<All> {

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param node     Node owning this Subscriber.
        PropertyObserver(Node& node) : m_node(node) {}

        /// Called whenever a visible Property changed its value.
        void on_next(const AnyPublisher*, ...) final
        {
            TheGraph::AccessFor<Node>::mark_dirty(m_node.shared_from_this());
        }

        // fields ---------------------------------------------------------- //
    private:
        /// Node owning this Subscriber.
        Node& m_node;
    };

    // flags ------------------------------------------------------------------

    /// Total bumber of flags on a Node (as many as fit into a pointer).
    static constexpr const size_t s_flag_count = bitsizeof<void*>();

    /// Internal Flags.
    enum class InternalFlags : uchar {
        FINALIZED,
        ENABLED,
        VISIBLE,
        __LAST,
    };

    /// Number of flags reserved for internal usage.
    static constexpr const size_t s_internal_flag_count = to_number(InternalFlags::__LAST);
    static_assert(s_internal_flag_count <= s_flag_count);

    // methods --------------------------------------------------------------------------------- //
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
    /// @param name     Node-unique name of the Property.
    /// @returns        Handle to the requested Property.
    /// @throws NoSuchPropertyError
    template<class T>
    PropertyHandle<T> get_property(const std::string& name) const
    {
        AnyPropertyPtr property = _get_property(name);
        if (!property) {
            NOTF_THROW(NoSuchPropertyError, "Node \"{}\" has no Property called \"{}\"", get_name(), name);
        }

        PropertyPtr<T> typed_property = std::dynamic_pointer_cast<Property<T>>(std::move(property));
        if (!typed_property) {
            NOTF_THROW(NoSuchPropertyError,
                       "Property \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), property->get_type_name(), type_name<T>());
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
    /// @throws OutOfBounds    If the index is out-of-bounds or the child Node is of the wrong type.
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

    // flags ------------------------------------------------------------------

    /// Returns the number of user definable flags of a Node on this system.
    static constexpr size_t get_user_flag_count()
    {
        return bitset_size_v<decltype(Node::m_flags)> - s_internal_flag_count;
    }

    /// Tests a user defineable flag on this Node.
    /// @param index            Index of the user flag.
    /// @returns                True iff the flag is set.
    /// @throws OutOfBounds   If index >= user flag count.
    bool is_user_flag_set(size_t index);

    /// Sets or unsets a user flag.
    /// @param index            Index of the user flag.
    /// @param value            Whether to set or to unser the flag.
    /// @throws OutOfBounds   If index >= user flag count.
    void set_user_flag(size_t index, bool value = true);

protected:
    /// Implementation specific query of a Property.
    /// @param name     Node-unique name of the Property.
    virtual AnyPropertyPtr _get_property(const std::string& name) const = 0;

    /// Calculates the combined hash value of all Properties.
    virtual size_t _calculate_property_hash() const = 0;

    /// Creates and adds a new child to this node.
    /// @param parent   Parent of the Node, must be `this` (is used for type checking).
    /// @param args     Arguments that are forwarded to the constructor of the child.
    /// @throws InternalError   If you pass anything else but `this` as the parent.
    template<class Child, class Parent, class... Args,
             class = std::enable_if_t<detail::can_node_parent<Parent, Child>()>>
    NewNode _create_child(Parent* parent, Args&&... args)
    {
        if (NOTF_UNLIKELY(parent != this)) {
            NOTF_THROW(InternalError, "Node::_create_child cannot be used to create children of other Nodes.");
        }

        auto child = std::static_pointer_cast<Node>(std::make_shared<Child>(parent, std::forward<Args>(args)...));
        child->_finalize();

        _write_children().emplace_back(child);
        TheGraph::AccessFor<Node>::register_node(child);
        return child;
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

    /// Deletes the modified child list copy, if one exists.
    void _clear_modified_children() { m_modified_children.reset(); }

    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    std::shared_ptr<PropertyObserver>& _get_property_observer() { return m_property_observer; }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return m_flags[to_number(InternalFlags::FINALIZED)]; }

private:
    /// All children of the parent.
    const ChildList& _read_siblings() const;

    /// Finalizes this Node.
    /// Called on every new Node instance right after the Constructor of the most derived class has finished.
    void _finalize() { m_flags[to_number(InternalFlags::FINALIZED)] = true; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Parent of this Node.
    Node* m_parent;

    /// Name of this Node. Is empty until requested for the first time or set using `set_name`.
    mutable std::string_view m_name;

    /// All children of this Node, ordered from back to front (later Nodes are drawn on top of earlier ones).
    ChildList m_children;

    /// Pointer to a modified copy of the list of children, if it was modified while the Graph was frozen.
    std::unique_ptr<ChildList> m_modified_children;

    /// Hash of all Property values of this Node.
    size_t m_property_hash;

    /// Combines the Property hash with the Node hashes of all children in order.
    size_t m_node_hash = 0;

    /// Additional flags.
    /// Is more compact than multiple booleans and offers per-node user-definable flags for free.
    /// Contains both internal and user-definable flags.
    std::bitset<s_flag_count> m_flags;

    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    std::shared_ptr<PropertyObserver> m_property_observer = std::make_shared<PropertyObserver>(*this);
};

// node accessors =================================================================================================== //

template<>
class Accessor<Node, NodeMasterHandle> {
    friend NodeMasterHandle;

    /// Lets a NodeMasterHandle remove the managed Node on destruction.
    static void remove(const NodePtr& node)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        node->_get_parent()->_remove_child(node);
    }
};

template<>
class Accessor<Node, AnyRootNode> {
    friend AnyRootNode;

    /// Finalizes a given (Root)Node.
    static void finalize(Node& node) { node._finalize(); }
};

NOTF_CLOSE_NAMESPACE
