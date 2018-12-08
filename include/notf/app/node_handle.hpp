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

    // properties -------------------------------------------------------------

    using NodeType::get;
    using NodeType::set;
    using NodeType::connect_property;

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

    /// Implicitly cast to a TypedNodeHandle, if the Node Type derives from NodeType.
    /// Can be called multiple times.
    template<class T = NodeType, class = std::enable_if_t<std::is_base_of_v<Node, T>>>
    operator TypedNodeHandle<T>() const {
        return TypedNodeHandle<T>(m_node.lock());
    }

    /// Implicit cast to a NodeOwner.
    /// Must only be called once.
    /// @throws HandleExpiredError  If called more than once or the Node has already expired.
    template<class T = NodeType, class = std::enable_if_t<std::is_base_of_v<Node, T>>>
    operator TypedNodeOwner<T>() {
        return _to_node_owner<T>();
    }

    /// Explicit conversion to a TypedNodeHandle.
    /// Is useful when you don't want to type the name:
    ///     auto owner = parent->create_child<VeryLongNodeName>(...).to_handle();
    auto to_handle() const { return operator TypedNodeHandle<NodeType>(); }

    /// Explicit conversion to a NodeType.
    /// Is useful when you don't want to type the name:
    ///     auto owner = parent->create_child<VeryLongNodeName>(...).to_owner();
    /// @throws HandleExpiredError  If called more than once or the Node has already expired.
    auto to_owner() { return operator TypedNodeOwner<NodeType>(); }

private:
    template<class T>
    TypedNodeOwner<T> _to_node_owner() {
        auto node = m_node.lock();
        if (!node) { NOTF_THROW(HandleExpiredError, "Cannot create a NodeOwner for a Node that is already expired"); }
        m_node.reset();
        return TypedNodeOwner<T>(std::move(node));
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
class TypedNodeHandle {

    static_assert(std::is_base_of_v<Node, NodeType>, "NodeType must be a type derived from Node");

    friend ::std::hash<TypedNodeHandle<NodeType>>;

    template<class>
    friend class TypedNodeHandle; // befriend all other TypedNodeHandles so you can copy their node

    friend Accessor<TypedNodeHandle, Node>;
    friend Accessor<TypedNodeHandle, detail::Graph>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TypedNodeHandle);

    /// Type of Node.
    using node_t = NodeType;

private:
    /// Short name of this type.
    using this_t = TypedNodeHandle;

    /// Interface determining which methods of the Node type are forwarded.
    using Interface = detail::NodeHandleInterface<NodeType>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    TypedNodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    TypedNodeHandle(std::shared_ptr<NodeType> node) : m_node(std::move(node)) {}

    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    TypedNodeHandle(std::shared_ptr<T> node) : m_node(std::static_pointer_cast<NodeType>(std::move(node))) {}
    /// @}

    /// Assignment operator.
    /// @param other    Other NodeHandle to copy.
    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    TypedNodeHandle& operator=(TypedNodeHandle<T> other) {
        m_node = std::static_pointer_cast<NodeType>(other.m_node.lock());
        return *this;
    }

    /// Implicit conversion to an (untyped) NodeHandle.
    operator NodeHandle() { return NodeHandle(std::static_pointer_cast<Node>(m_node.lock())); }

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
bool operator==(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept {
    return weak_ptr_equal(lhs.m_node, rhs.m_node);
}
template<class LeftType, class RightType>
bool operator!=(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept {
    return !operator==(lhs, rhs);
}

/// Less-than operator with another NodeHandle.
template<class LeftType, class RightType>
bool operator<(const TypedNodeHandle<LeftType>& lhs, const TypedNodeHandle<RightType>& rhs) noexcept {
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
    TypedNodeOwner(std::shared_ptr<NodeType> node) : TypedNodeHandle<NodeType>(std::move(node)) {}

    /// Move assignment operator.
    TypedNodeOwner& operator=(TypedNodeOwner&& other) {
        if (auto node = this->m_node.lock(); node) { node->remove(); };
        this->m_node = std::move(other.m_node);
        return *this;
    }

    /// Destructor.
    /// Note that the destruction of a Node requires the Graph mutex. Normally (if you store the Handle on the parent
    /// Node or some other Node in the Graph) this does not block, but it might if the mutex is not already held by this
    /// thread.
    ~TypedNodeOwner() {
        if (auto node = this->m_node.lock(); node) { node->remove(); };
    }
};

// node handle accessors ============================================================================================ //

template<class NodeType>
class Accessor<TypedNodeHandle<NodeType>, detail::Graph> {
    friend detail::Graph;

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
    constexpr size_t operator()(const notf::TypedNodeHandle<NodeType>& handle) const noexcept {
        return notf::hash_mix(notf::to_number(handle._get_ptr()));
    }
};

} // namespace std
