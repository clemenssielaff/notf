#pragma once

#include "notf/meta/pointer.hpp"
#include "notf/meta/stringtype.hpp"

#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "./app.hpp"

NOTF_OPEN_NAMESPACE

// node handle base ================================================================================================= //

namespace detail {

/// Base class of all NodeHandles.
/// Offers static functions that allow us to not include node.hpp in this header.
struct AnyNodeHandle {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// The Graph mutex.
    static RecursiveMutex& _get_graph_mutex();

    /// Removes the given Node from its parent Node.
    static void _remove_from_parent(NodePtr&& node);
};

} // namespace detail

// typed node handle ================================================================================================ //

/// Members common to NodeHandle and NodeOwner.
template<class NodeType>
class TypedNodeHandle : public detail::AnyNodeHandle {

    static_assert(std::is_base_of_v<Node, NodeType>, "NodeType must be a type derived from Node");

    friend ::std::hash<TypedNodeHandle<NodeType>>;

    template<class>
    friend class TypedNodeHandle; // befriend all other TypedNodeHandles so you can copy their node

    friend Accessor<TypedNodeHandle, Node>;
    friend Accessor<TypedNodeHandle, TheGraph>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TypedNodeHandle);

    /// Type of Node.
    using node_t = NodeType;

private:
    /// Short name of this type.
    using this_t = TypedNodeHandle;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    TypedNodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    TypedNodeHandle(std::shared_ptr<NodeType> node) : m_node(std::move(node)) {}

    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    TypedNodeHandle(std::shared_ptr<T> node) : m_node(std::static_pointer_cast<NodeType>(std::move(node)))
    {}
    /// @}

    /// Assignment operator.
    /// @param other    Other NodeHandle to copy.
    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    TypedNodeHandle& operator=(TypedNodeHandle<T> other)
    {
        m_node = std::static_pointer_cast<NodeType>(other.m_node.lock());
        return *this;
    }

    /// Implicit conversion to an (untyped) NodeHandle.
    operator NodeHandle() { return NodeHandle(std::static_pointer_cast<Node>(m_node.lock())); }

    /// @{
    /// Checks whether the NodeHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_node.expired(); }
    explicit operator bool() const { return !is_expired(); }
    /// @}

    /// The Uuid of this Node.
    /// @throws     HandleExpiredError  If the Handle has expired.
    Uuid get_uuid() const { return _get_node()->get_uuid(); }

    /// The Graph-unique name of this Node.
    /// @returns    Name of this Node. Is an l-value because the name of the Node may change.
    /// @throws     HandleExpiredError  If the Handle has expired.
    std::string get_name() const { return _get_node()->get_name(); }

    /// A 4-syllable mnemonic name, based on this Node's Uuid.
    /// Is not guaranteed to be unique, but collisions are unlikely with 100^4 possibilities.
    std::string get_default_name() const { return _get_node()->get_default_name(); }

    /// Run time access to a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @returns        Handle to the requested Property.
    /// @throws         NoSuchPropertyError
    template<class T>
    PropertyHandle<T> get_property(const std::string& name) const
    {
        return _get_node()->template get_property<T>(name);
    }

    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    /// @returns        Typed handle to the requested Property.
    template<char... Cs, class X = NodeType, class = std::enable_if_t<detail::is_compile_time_node_v<X>>>
    auto get_property(StringType<char, Cs...> name) const
    {
        return this->_get_node()->get_property(std::forward<decltype(name)>(name));
    }

    // hierarchy --------------------------------------------------------------

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    void set_name(const std::string& name)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->set_name(name);
    }

    /// The parent of this Node.
    NodeHandle get_parent()
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->get_parent();
    }

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor         Potential ancestor to verify.
    bool has_ancestor(const NodeHandle& ancestor) const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->has_ancestor(ancestor);
    }

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// If the handle passed in is expired, the returned handle will also be expired.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws HierarchyError  If there is no common ancestor.
    NodeHandle get_common_ancestor(const NodeHandle& other)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->get_common_ancestor(other);
    }

    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @returns    Typed handle of the first ancestor with the requested type, can be empty if none was found.
    template<class T, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle get_first_ancestor()
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->template get_first_ancestor<T>();
    }

    /// The number of direct children of this Node.
    size_t get_child_count() const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->get_child_count();
    }

    /// Returns a handle to a child Node at the given index.
    /// Index 0 is the node furthest back, index `size() - 1` is the child drawn at the front.
    /// @param index    Index of the Node.
    /// @returns        The requested child Node.
    /// @throws OutOfBounds    If the index is out-of-bounds or the child Node is of the wrong type.
    NodeHandle get_child(const size_t index)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->get_child(index);
    }

    // z-order ----------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    bool is_in_front() const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->is_in_front();
    }

    /// Checks if this Node is behind all of its siblings.
    bool is_in_back() const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->is_in_back();
    }

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_before(const NodeHandle& sibling) const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->is_before(sibling);
    }

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_behind(const NodeHandle& sibling) const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->is_behind(sibling);
    }

    /// Moves this Node in front of all of its siblings.
    void stack_front()
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->stack_front();
    }

    /// Moves this Node behind all of its siblings.
    void stack_back()
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->stack_back();
    }

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_before(const NodeHandle& sibling)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->stack_before(sibling);
    }

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_behind(const NodeHandle& sibling)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->stack_behind(sibling);
    }

    // flags ------------------------------------------------------------------

    /// Returns the number of user definable flags of a Node on this system.
    static constexpr size_t get_user_flag_count() noexcept { return NodeType::get_user_flag_count(); }

    /// Tests a user defineable flag on this Node.
    /// @param index            Index of the user flag.
    /// @returns                True iff the flag is set.
    /// @throws OutOfBounds   If index >= user flag count.
    bool is_user_flag_set(const size_t index) const
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        return _get_node()->is_user_flag_set(index);
    }

    /// Sets or unsets a user flag.
    /// @param index            Index of the user flag.
    /// @param value            Whether to set or to unser the flag.
    /// @throws OutOfBounds   If index >= user flag count.
    void set_user_flag(const size_t index, const bool value = true)
    {
        NOTF_GUARD(std::lock_guard(_get_graph_mutex()));
        _get_node()->set_user_flag(index, value);
    }

    // comparison -------------------------------------------------------------

    /// GCC requires these to be instantiated outside the class
    template<class LeftType, class RightType>
    friend bool operator==(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept;
    template<class LeftType, class RightType>
    friend bool operator!=(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept;
    template<class LeftType, class RightType>
    friend bool operator<(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept;

    /// Comparison with a NodePtr.
    friend bool operator==(const this_t& lhs, const NodePtr& rhs) noexcept { return lhs._get_ptr() == rhs.get(); }
    friend bool operator!=(const this_t& lhs, const NodePtr& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator==(const NodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() == rhs._get_ptr(); }
    friend bool operator!=(const NodePtr& lhs, const this_t& rhs) noexcept { return !(lhs == rhs); }

    /// Less-than operator with a NodePtr.
    friend bool operator<(const this_t& lhs, const NodePtr& rhs) noexcept { return lhs._get_ptr() < rhs.get(); }
    friend bool operator<(const NodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() < rhs._get_ptr(); }

protected:
    /// Raw pointer to the handled Node (does not check if the Node is still alive).
    const Node* _get_ptr() const { return static_cast<const Node*>(raw_from_weak_ptr(m_node)); }

    /// @{
    /// Locks and returns an owning pointer to the handled Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::shared_ptr<NodeType> _get_node()
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }
    std::shared_ptr<const NodeType> _get_node() const
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }
    /// }@

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// The handled Node, non owning.
    std::weak_ptr<NodeType> m_node;
};

/// Equality comparison with another NodeHandle.
template<class LeftType, class RightType>
bool operator==(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept
{
    return weak_ptr_equal(lhs.m_node, rhs.m_node);
}
template<class LeftType, class RightType>
bool operator!=(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept
{
    return !operator==(lhs, rhs);
}

/// Less-than operator with another NodeHandle.
template<class LeftType, class RightType>
bool operator<(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept
{
    return lhs.m_node.owner_before(rhs.m_node);
}

// typed node owner ================================================================================================= //

/// Special NodeHandle type that is unique per Node instance and removes the Node when itself goes out of scope.
/// If the Node has already been removed by then, the destructor does nothing.
template<class NodeType>
class TypedNodeOwner : public TypedNodeHandle<NodeType> {

    friend struct ::std::hash<NodeOwner>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(TypedNodeOwner);

    /// Default (empty) Constructor.
    TypedNodeOwner() = default;

    /// Value Constructor.
    /// @param node     Node to handle.
    /// @throws NotValidError   If the given Node pointer is empty.
    TypedNodeOwner(valid_ptr<std::shared_ptr<NodeType>> node) : TypedNodeHandle<NodeType>(node) {}

    /// Move assignment operator.
    TypedNodeOwner& operator=(TypedNodeOwner&& other) = default;

    /// Destructor.
    /// Note that the destruction of a Node requires the Graph mutex. Normally (if you store the Handle on the parent
    /// Node or some other Node in the Graph) this does not block, but it might if the mutex is not already held by this
    /// thread.
    ~TypedNodeOwner() { this->_remove_from_parent(this->m_node.lock()); }
};

// node handle accessors
// =================================================================================================== //

template<class NodeType>
class Accessor<TypedNodeHandle<NodeType>, TheGraph> {
    friend TheGraph;

    /// Unwraps the shared_ptr contained in a NodeHandle.
    static NodePtr get_node_ptr(const NodeHandle& node) { return node.m_node.lock(); }
};

template<class NodeType>
class Accessor<TypedNodeHandle<NodeType>, Node> {
    friend Node;

    /// Unwraps the shared_ptr contained in a NodeHandle.
    static NodePtr get_node_ptr(const NodeHandle& node) { return node.m_node.lock(); }
};

NOTF_CLOSE_NAMESPACE

// std::hash specializations ======================================================================================== //

namespace std {

template<class NodeType>
struct hash<notf::TypedNodeHandle<NodeType>> {
    constexpr size_t operator()(const notf::TypedNodeHandle<NodeType>& handle) const noexcept
    {
        return notf::hash_mix(notf::to_number(handle._get_ptr()));
    }
};

} // namespace std
