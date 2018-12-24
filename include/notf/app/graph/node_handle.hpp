#pragma once

#include "notf/meta/hash.hpp"
#include "notf/meta/stringtype.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// node handle interface ============================================================================================ //

namespace detail {

/// Interface common to all Node subclasses.
template<class NodeType>
struct NodeHandleBaseInterface : protected NodeType {

    using NodeType::to_handle;

    // properties -------------------------------------------------------------

    using NodeType::connect_property;
    using NodeType::get;
    using NodeType::set;

    // signals / slots --------------------------------------------------------

    using NodeType::call;
    using NodeType::connect_signal;
    using NodeType::connect_slot;

    // hierarchy --------------------------------------------------------------

    using NodeType::get_child;
    using NodeType::get_child_count;
    using NodeType::get_common_ancestor;
    using NodeType::get_first_ancestor;
    using NodeType::get_parent;
    using NodeType::has_ancestor;
    using NodeType::set_name;

    // z-order ----------------------------------------------------------------

    using NodeType::is_before;
    using NodeType::is_behind;
    using NodeType::is_in_back;
    using NodeType::is_in_front;
    using NodeType::stack_back;
    using NodeType::stack_before;
    using NodeType::stack_behind;
    using NodeType::stack_front;
};

/// Generic Node Handle Interface.
/// Is used as fallback and exposes the methods common to all NodeHandles.
/// If you want to have special TypedNodeHandles, you can specialize this template and still derive from the base
/// interface. Then, you can simply add your special methods to the common ones, without listing them explicitly:
///
///     template<>
///     struct NodeHandleInterface<SuperNode> : public NodeHandleBaseInterface<SuperNode> {
///         // automatically exposes all methods listed in the common interface
///         // ...
///         using SuperNode::additional_method; // only exists on `SuperNode`.
///     };
///
template<class T>
struct NodeHandleInterface : public NodeHandleBaseInterface<T> {};

// new node ========================================================================================================= //

/// Type returned by Node::_create_child.
/// Can be cast to a NodeOwner (once), but can also be safely ignored without the Node being erased immediately.
template<class NodeType>
class NewNode {

    // methods ----------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(NewNode);

    /// Constructor.
    /// @param node     The newly created Node.
    NewNode(std::shared_ptr<NodeType>&& node) : m_node(std::move(node)) {}

    /// Implicitly cast to a NodeHandle, if the Node Type derives from NodeType.
    /// Can be called multiple times.
    template<class T = NodeType, class = std::enable_if_t<std::is_base_of_v<AnyNode, T>>>
    operator NodeHandle<T>() const {
        return NodeHandle<T>(m_node.lock());
    }

    /// Implicit cast to a NodeOwner.
    /// Must only be called once.
    /// @throws HandleExpiredError  If called more than once or the Node has already expired.
    template<class T = NodeType, class = std::enable_if_t<std::is_base_of_v<AnyNode, T>>>
    operator NodeOwner<T>() {
        return _to_node_owner<T>();
    }

    /// Explicit conversion to a NodeHandle.
    /// Is useful when you don't want to type the name:
    ///     auto owner = parent->create_child<VeryLongNodeName>(...).to_handle();
    NodeHandle<NodeType> to_handle() const { return NodeHandle<NodeType>(m_node.lock()); }

    /// Explicit conversion to a NodeType.
    /// Is useful when you don't want to type the name:
    ///     auto owner = parent->create_child<VeryLongNodeName>(...).to_owner();
    /// @throws HandleExpiredError  If called more than once or the Node has already expired.
    NodeOwner<NodeType> to_owner() { return _to_node_owner<NodeType>(); }

private:
    template<class T>
    NodeOwner<T> _to_node_owner() {
        auto node = m_node.lock();
        if (!node) { NOTF_THROW(HandleExpiredError, "Cannot create a NodeOwner for a Node that is already expired"); }
        m_node.reset();
        return NodeOwner<T>(std::move(node));
    }

    // fields ------------------------------------------------------------------------------ //
private:
    /// The newly created Node.
    /// Is held as a weak_ptr because the user might (foolishly) decide to store this object instead of using for
    /// casting only, and we don't want to keep the Node alive for longer than its parent is.
    std::weak_ptr<NodeType> m_node;
};

} // namespace detail

// typed node handle ================================================================================================ //

/// Members common to NodeHandle and NodeOwner.
/// All methods accessible via `.` can be called from anywhere, methods accessible via `->` must only be called from the
/// UI thread.
template<class NodeType>
class NodeHandle {

    static_assert(std::is_base_of_v<AnyNode, NodeType>, "NodeType must be a type derived from Node");

    friend ::std::hash<NodeHandle<NodeType>>;

    template<class>
    friend class NodeHandle; // befriend all other TypedNodeHandles so you can copy their node

    friend Accessor<NodeHandle, AnyNode>;
    friend Accessor<NodeHandle, AnyWidget>;
    friend Accessor<NodeHandle, detail::Graph>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(NodeHandle);

    /// Type of Node.
    using node_t = NodeType;

private:
    /// Short name of this type.
    using this_t = NodeHandle;

    /// Interface determining which methods of the Node type are forwarded.
    using Interface = detail::NodeHandleInterface<NodeType>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    NodeHandle() = default;
    NodeHandle(const NodeHandle&) = default;

    // Why do NodeHandles not have a default move constructor?
    //
    // There seems to be a curious edge case here where moving the handle from one thread onto another causes the
    // LLVM Thread Sanitizer to report data races. I think they are spurious, because the standard says that operations
    // on the control block are thread safe (even with non-atomic weak_ and shared_ptrs), but if there is just the
    // faintest possibility that they are not and there are in fact cases where moving a weak_ptr may somehow interfere
    // with the lifetime of the weak_pointer's control block, it is better to be sure.
    //
    // There are two ways that seem to work.
    // First: explicitly define a move constructor and only copy the other's node:
    //
    //     NodeHandle(NodeHandle&& other) noexcept : m_node(other.m_node) {}
    //
    // Second, do not define a move constructor - not even a "default" one. This seems to be different from "deleting"
    // the default move constructor, even though C++ shouldn't be allowed to create one in the first place (because we
    // have a user-declared copy constructor)? Anyway, it works and since it leaves open the possibility that the
    // compiler is smart enough to handle the situation without us telling it to copy the weak_ptr, I opt for 2.

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(std::shared_ptr<NodeType> node) : m_node(std::move(node)) {}
    NodeHandle(std::weak_ptr<NodeType> node) : m_node(std::move(node)) {}

    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    NodeHandle(std::shared_ptr<T> node) : m_node(std::static_pointer_cast<NodeType>(std::move(node))) {}
    /// @}

    /// @{
    /// Assignment operator.
    /// @param other    Other NodeHandle to copy / move from.
    NodeHandle& operator=(NodeHandle&&) = default;
    NodeHandle& operator=(const NodeHandle& other) = default;
    /// @}

    /// Implicit conversion to an (untyped) NodeHandle.
    operator AnyNodeHandle() { return AnyNodeHandle(std::static_pointer_cast<AnyNode>(m_node.lock())); }

    // identification ---------------------------------------------------------

    /// @{
    /// Checks whether the NodeHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_node.expired(); }
    explicit operator bool() const { return !is_expired(); }
    /// @}

    /// Uuid of this Node.
    auto get_uuid() const { return _get_node()->get_uuid(); }

    /// The Graph-unique name of this Node.
    std::string get_name() const { return _get_node()->get_name(); }

    // access -----------------------------------------------------------------

    Interface* operator->() { return _get_interface(); }
    const Interface* operator->() const { return _get_interface(); }

    // comparison -------------------------------------------------------------

    /// GCC requires these to be instantiated outside the class
    template<class LeftType, class RightType>
    friend bool operator==(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept;
    template<class LeftType, class RightType>
    friend bool operator!=(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept;
    template<class LeftType, class RightType>
    friend bool operator<(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept;

    /// Comparison with a AnyNodePtr.
    friend bool operator==(const this_t& lhs, const AnyNodePtr& rhs) noexcept { return lhs._get_ptr() == rhs.get(); }
    friend bool operator!=(const this_t& lhs, const AnyNodePtr& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator==(const AnyNodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() == rhs._get_ptr(); }
    friend bool operator!=(const AnyNodePtr& lhs, const this_t& rhs) noexcept { return !(lhs == rhs); }

    /// Less-than operator with a AnyNodePtr.
    friend bool operator<(const this_t& lhs, const AnyNodePtr& rhs) noexcept { return lhs._get_ptr() < rhs.get(); }
    friend bool operator<(const AnyNodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() < rhs._get_ptr(); }

protected:
    /// Raw pointer to the handled Node (does not check if the Node is still alive).
    const AnyNode* _get_ptr() const { return static_cast<const AnyNode*>(raw_from_weak_ptr(m_node)); }

    /// Returns an interface pointer.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::shared_ptr<NodeType> _get_node() const {
        auto nodeptr = m_node.lock();
        if (!nodeptr) { NOTF_THROW(HandleExpiredError, "Node Handle is expired"); }
        return nodeptr;
    }

    /// Returns an interface pointer.
    /// @throws HandleExpiredError  If the Handle has expired.
    /// @throws ThreadError         If the current thread is not the UI thread.
    Interface* _get_interface() const {
        if (!this_thread::is_the_ui_thread()) {
            NOTF_THROW(ThreadError, "NodeHandles may only be modified from the UI thread");
        }
        return reinterpret_cast<Interface*>(_get_node().get());
    }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// The handled Node, non owning.
    std::weak_ptr<NodeType> m_node;
};

/// Equality comparison with another NodeHandle.
template<class LeftType, class RightType>
bool operator==(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept {
    return weak_ptr_equal(lhs.m_node, rhs.m_node);
}
template<class LeftType, class RightType>
bool operator!=(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept {
    return !operator==(lhs, rhs);
}

/// Less-than operator with another NodeHandle.
template<class LeftType, class RightType>
bool operator<(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept {
    return lhs.m_node.owner_before(rhs.m_node);
}

// typed node owner ================================================================================================= //

/// Special NodeHandle type that is unique per Node instance and removes the Node when itself goes out of scope.
/// If the Node has already been removed by then, the destructor does nothing.
template<class NodeType>
class NodeOwner : public NodeHandle<NodeType> {

    friend struct ::std::hash<AnyNodeOwner>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(NodeOwner);

    /// Default (empty) Constructor.
    NodeOwner() = default;

    /// Value Constructor.
    /// @param node     Node to handle.
    NodeOwner(std::shared_ptr<NodeType> node) : NodeHandle<NodeType>(std::move(node)) {}

    /// Move assignment operator.
    NodeOwner& operator=(NodeOwner&& other) {
        if (auto node = this->m_node.lock(); node) { node->remove(); };
        this->m_node = std::move(other.m_node);
        return *this;
    }

    /// Destructor.
    /// Note that the destruction of a Node requires the Graph mutex. Normally (if you store the Handle on the parent
    /// Node or some other Node in the Graph) this does not block, but it might if the mutex is not already held by this
    /// thread.
    ~NodeOwner() {
        if (auto node = this->m_node.lock(); node) { node->remove(); };
    }
};

// node handle accessors ============================================================================================ //

template<class NodeType>
class Accessor<NodeHandle<NodeType>, detail::Graph> {
    friend detail::Graph;

    static AnyNodePtr get_node_ptr(const AnyNodeHandle& node) { return node.m_node.lock(); }
};

template<class NodeType>
class Accessor<NodeHandle<NodeType>, AnyNode> {
    friend AnyNode;

    static AnyNodePtr get_node_ptr(const AnyNodeHandle& node) { return node.m_node.lock(); }
};

template<class NodeType>
class Accessor<NodeHandle<NodeType>, AnyWidget> {
    friend AnyWidget;

    static AnyNodePtr get_node_ptr(const AnyNodeHandle& node) { return node.m_node.lock(); }
};

NOTF_CLOSE_NAMESPACE

// std::hash specializations ======================================================================================== //

namespace std {

template<class NodeType>
struct hash<notf::NodeHandle<NodeType>> {
    constexpr size_t operator()(const notf::NodeHandle<NodeType>& handle) const noexcept {
        return notf::hash_mix(notf::to_number(handle._get_ptr()));
    }
};

} // namespace std
