#pragma once

#include "notf/common/bitset.hpp"
#include "notf/common/tuple.hpp"

#include "./property_handle.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

// can node parent ------------------------------------------------------------

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
constexpr bool can_node_parent() noexcept
{
    // both A and B must be derived from Node
    if constexpr (std::negation_v<std::conjunction<std::is_base_of<Node, A>, std::is_base_of<Node, B>>>) {
        return false;
    }
    else { // explicit else so the code coverage doesn't pretend like these lines are not covered...

        // if A has a list of explicitly allowed child types, B must be in it
        if constexpr (has_allowed_child_types<A>::value) {
            if (!is_derived_from_one_of_tuple_v<B, typename A::allowed_child_types>) { return false; }
        }
        // ... otherwise, if A has a list of explicitly forbidden child types, B must NOT be in it
        else if constexpr (has_forbidden_child_types<A>::value) {
            if (is_derived_from_one_of_tuple_v<B, typename A::forbidden_child_types>) { return false; }
        }

        // if B has a list of explicitly allowed parent types, A must be in it
        else if constexpr (has_allowed_parent_types<B>::value) {
            if (!is_derived_from_one_of_tuple_v<A, typename B::allowed_parent_types>) { return false; }
        }
        // ... otherwise, if B has a list of explicitly forbidden parent types, A must NOT be in it
        else if constexpr (has_forbidden_parent_types<B>::value) {
            if (is_derived_from_one_of_tuple_v<A, typename B::forbidden_parent_types>) { return false; }
        }
    }

    return true;
}

} // namespace detail

// node ============================================================================================================= //

/// Base class for both RunTimeNode and CompileTimeNode.
/// The Node interface is used internally only, meaning only through Node subclasses or by other notf objects.
/// All user-access should occur through NodeHandle instances. This way, we can rely on certain preconditions to be met
/// for the user of this interface; first and foremost whether the Graph mutex is locked when a method is called.
class Node : public std::enable_shared_from_this<Node> {

    friend Accessor<Node, detail::AnyNodeHandle>;
    friend Accessor<Node, AnyRootNode>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Node);

    /// Exception thrown by `get_property` if either the name of the type of the requested Property is wrong.
    NOTF_EXCEPTION_TYPE(NoSuchPropertyError);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(FinalizedError);

    /// If two Nodes have no common ancestor.
    NOTF_EXCEPTION_TYPE(HierarchyError);

private:
    /// Owning list of child Nodes, ordered from back to front.
    using ChildList = std::vector<NodePtr>;

    // new node ---------------------------------------------------------------

    /// Type returned by Node::_create_child.
    /// Can once (implicitly) be cast to a NodeOwner, but can also be safely ignored without the Node being erased
    /// immediately.
    template<class NodeType>
    class NewNode {

        // methods ----------------------------------------------------------------------------- //
    public:
        NOTF_NO_COPY_OR_ASSIGN(NewNode);

        /// Constructor.
        /// @param node     The newly created Node.
        NewNode(std::shared_ptr<NodeType>&& node) : m_node(std::move(node)) {}

        /// Implicitly cast to a CompileTimeNodeHandle, if the Node Type derives from CompileTimeNode.
        /// Can be called multiple times.
        operator TypedNodeHandle<NodeType>() { return TypedNodeHandle<NodeType>(m_node.lock()); }

        /// Implicit cast to a NodeOwner.
        /// Must only be called once.
        /// @throws ValueError  If called more than once or the Node has already expired.
        operator TypedNodeOwner<NodeType>() { return _to_node_owner<NodeType>(); }

        /// Implicit cast to a NodeHandle.
        /// Can be called multiple times.
        operator NodeHandle() { return NodeHandle(m_node.lock()); }

        /// Implicit cast to a NodeOwner.
        /// Must only be called once.
        /// @throws ValueError  If called more than once or the Node has already expired.
        operator NodeOwner() { return _to_node_owner<Node>(); }

    private:
        template<class T>
        TypedNodeOwner<T> _to_node_owner()
        {
            if (auto node = m_node.lock()) {
                m_node.reset();
                return TypedNodeOwner<T>(std::move(node));
            }
            else {
                NOTF_THROW(ValueError, "Cannot create a NodeOwner for a Node that is already expired");
            }
        }

        // fields ------------------------------------------------------------------------------ //
    private:
        /// The newly created Node.
        /// Is held as a weak_ptr because the user might (foolishly) decide to store this object instead of using for
        /// casting only, and we don't want to keep the Node alive for longer than its parent is.
        std::weak_ptr<NodeType> m_node;
    };

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
    using PropertyObserverPtr = std::shared_ptr<PropertyObserver>;

    // flags ------------------------------------------------------------------

    /// Total bumber of flags on a Node (as many as fit into a pointer).
    using Flags = std::bitset<bitsizeof<void*>()>;

    /// Internal Flags.
    enum class InternalFlags : uchar {
        FINALIZED,
        ENABLED,
        VISIBLE,
        __LAST,
    };
    static_assert(to_number(InternalFlags::__LAST) <= bitset_size_v<Flags>);

    /// Number of flags reserved for internal usage.
    static constexpr size_t s_internal_flag_count = to_number(InternalFlags::__LAST);

    // data -------------------------------------------------------------------

    /// If a Node is modified while the Graph is frozen, all modifications will occur on a copy of the Node's Data, so
    /// the render thread will still see everything as it was when the Graph was frozen.
    /// Since it will be a lot more common for a single Node to be modified many times than it is for many Nodes to be
    /// modified a single time, it is advantageous to have a single unused pointer in many Nodes and a few unneccessary
    /// data copies on some, than it is to have many unused pointers on most Nodes.
    struct Data {
        /// Value Constructor.
        /// Initializes all data, so we can be sure that all of it is valid if a modified data copy exists.
        Data(valid_ptr<Node*> parent, const ChildList& children, Flags flags)
            : parent(parent), children(std::make_unique<ChildList>(children)), flags(std::move(flags))
        {}

        /// Modified parent of this Node, if it was moved.
        valid_ptr<Node*> parent;

        /// Modified children of this Node, should they have been modified.
        valid_ptr<std::unique_ptr<ChildList>> children;

        /// Modified flags of this Node.
        Flags flags;
    };
    using DataPtr = std::unique_ptr<Data>;

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
    Uuid get_uuid() const noexcept { return m_uuid; }

    /// The Graph-unique name of this Node.
    /// @returns    Name of this Node. Is an l-value because the name of the Node may change.
    std::string get_name() const;

    /// A 4-syllable mnemonic name, based on this Node's Uuid.
    /// Is not guaranteed to be unique, but collisions are unlikely with 100^4 possibilities.
    std::string get_default_name() const;

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    void set_name(const std::string& name);

    /// Run time access to a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @returns        Handle to the requested Property.
    /// @throws         NoSuchPropertyError
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
    NodeHandle get_parent();

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor         Potential ancestor to verify.
    bool has_ancestor(const NodeHandle& ancestor) const;

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// If the handle passed in is expired, the returned handle will also be expired.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws HierarchyError  If there is no common ancestor.
    NodeHandle get_common_ancestor(const NodeHandle& other);

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
    NodeHandle get_child(size_t index);

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
    static constexpr size_t get_user_flag_count() { return bitset_size_v<Flags> - s_internal_flag_count; }

    /// Tests a user defineable flag on this Node.
    /// @param index            Index of the user flag.
    /// @returns                True iff the flag is set.
    /// @throws OutOfBounds   If index >= user flag count.
    bool is_user_flag_set(size_t index) const;

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
    NewNode<Child> _create_child(Parent* parent, Args&&... args)
    {
        if (NOTF_UNLIKELY(parent != this)) {
            NOTF_THROW(InternalError, "Node::_create_child cannot be used to create children of other Nodes.");
        }

        auto child = std::make_shared<Child>(parent, std::forward<Args>(args)...);

        { // register the new node with the graph and store it as child
            auto node = std::static_pointer_cast<Node>(child);
            node->_finalize();
            TheGraph::AccessFor<Node>::register_node(node);
            _write_children().emplace_back(std::move(node));
        }

        return child;
    }

    /// Removes a child from this node.
    /// @param handle   Handle of the node to remove.
    void _remove_child(NodeHandle child_handle);

    /// @{
    /// Access to the parent of this Node.
    /// Never creates a modified copy.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const Node* _get_parent(std::thread::id thread_id = std::this_thread::get_id()) const;
    Node* _get_parent(const std::thread::id thread_id = std::this_thread::get_id())
    {
        return const_cast<Node*>(const_cast<const Node*>(this)->_get_parent(thread_id));
    }
    /// @}

    /// Changes the parent of this Node by first adding it to the new parent and then removing it from its old one.
    /// @param new_parent_handle    New parent of this Node. If it is the same as the old, this method does nothing.
    void _set_parent(NodeHandle new_parent_handle);

    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    std::shared_ptr<PropertyObserver>& _get_property_observer() { return m_property_observer; }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return _is_flag_set(to_number(InternalFlags::FINALIZED)); }

private:
    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws HierarchyError  If there is no common ancestor.
    const Node* _get_common_ancestor(const Node* other) const;

    /// All children of this node, orded from back to front.
    /// Never creates a modified copy.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const ChildList& _read_children(std::thread::id thread_id = std::this_thread::get_id()) const;

    /// All children of this node, orded from back to front.
    /// Will create a modified copy the current list of children if there is no copy yet and the Graph is frozen.
    ChildList& _write_children();

    /// All children of the parent.
    const ChildList& _read_siblings() const;

    /// Finalizes this Node.
    /// Called on every new Node instance right after the Constructor of the most derived class has finished.
    /// Therefore, we do no have to ensure that the Graph is frozen etc.
    void _finalize() { m_flags[to_number(InternalFlags::FINALIZED)] = true; }

    /// Tests a flag on this Node.
    /// @param index            Index of the user flag.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    /// @returns                True iff the flag is set.
    bool _is_flag_set(size_t index, std::thread::id thread_id = std::this_thread::get_id()) const;

    /// Sets or unsets a flag on this Node.
    /// @param index            Index of the user flag.
    /// @param value            Whether to set or to unser the flag.
    void _set_flag(size_t index, bool value = true);

    /// Creates (if necessary) and returns the modified Data for this Node.
    Data& _ensure_modified_data();

    /// Marks this Node as dirty if it is finalized.
    void _mark_as_dirty();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Parent of this Node.
    Node* m_parent;

    /// All children of this Node, ordered from back to front (later Nodes are drawn on top of earlier ones).
    ChildList m_children;

    /// Name of this Node. Is empty until requested for the first time or set using `set_name`.
    std::string_view m_name;

    /// Additional flags, contains both internal and user-definable flags.
    Flags m_flags;

    /// Pointer to modified Data, should this Node have been modified while the Graph has been frozen.
    DataPtr m_modified_data;

    /// Hash of all Property values of this Node.
    size_t m_property_hash;

    /// Combines the Property hash with the Node hashes of all children in order.
    size_t m_node_hash = 0;

    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    PropertyObserverPtr m_property_observer = std::make_shared<PropertyObserver>(*this);
};

// node accessors =================================================================================================== //

template<>
class Accessor<Node, detail::AnyNodeHandle> {
    friend detail::AnyNodeHandle;

    /// Lets a NodeOwner remove the managed Node on destruction.
    static void remove(const NodePtr& node)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        NodePtr parent;
        if (auto weak_parent = node->_get_parent()->weak_from_this(); !weak_parent.expired()) {
            parent = weak_parent.lock();
        }
        else {
            // Often, a NodeOwner is stored on the parent Node itself.
            // In that case, it will be destroyed right after the parent's Node class destructor has finished, and
            // just before the next subclass' destructor begins. At that point, the `shared_ptr` wrapping the deepest
            // nested subclass, will have already been destroyed and all calls to `shared_from_this` or other
            // `shared_ptr` functions will throw.
            // Luckily, we can reliably test for this by checking if the `weak_ptr` contained in any Node through its
            // inheritance of `std::enable_shared_from_this` has expired. If so, we don't have to tell the parent to
            // remove the handled child, since the parent is about to be destroyed anyway and will take down all
            // children with it.
            return;
        }
        NOTF_ASSERT(parent);
        parent->_remove_child(node);
    }
};

template<>
class Accessor<Node, AnyRootNode> {
    friend AnyRootNode;

    /// Finalizes a given (Root)Node.
    static void finalize(Node& node) { node._finalize(); }
};

NOTF_CLOSE_NAMESPACE
