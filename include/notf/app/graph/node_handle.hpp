#pragma once

#include "notf/meta/hash.hpp"
#include "notf/meta/stringtype.hpp"

#include "notf/common/mutex.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// handle cast ====================================================================================================== //

/// Runtime-safe cast from one handle type to another.
/// @param from Handle to cast from.
/// @result     Handle to the same node but of the target type or an empty handle of the target type.
template<class Target, class T, class = std::enable_if_t<std::is_base_of_v<detail::NodeHandleBase, Target>>>
Target handle_cast(const NodeHandle<T>& from) {
    if (auto node = from.m_node.lock()) {
        return Target(std::dynamic_pointer_cast<typename Target::node_t>(std::move(node)));
    }
    return {};
}

// node handle interface ============================================================================================ //

namespace detail {

/// Interface common to all Node subclasses.
template<class NodeType>
struct NodeHandleBaseInterface : protected NodeType {

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

// node handle base ================================================================================================= //

/// Any Node Handle base class for identification.
struct NodeHandleBase {
    virtual ~NodeHandleBase() = default;

protected:
    /// Mutex used by all Handles to guard Handle destruction.
    /// See `~NodeHandle<T>` for details.
    static inline Mutex s_mutex;
};

} // namespace detail

// typed node handle ================================================================================================ //

/// Members common to NodeHandle and NodeOwner.
/// All methods accessible via `.` can be called from anywhere, methods accessible via `->` must only be called from the
/// UI thread.
template<class NodeType>
class NodeHandle : public detail::NodeHandleBase {

    static_assert(std::is_base_of_v<AnyNode, NodeType>, "NodeType must be a type derived from Node");

    friend ::std::hash<NodeHandle<NodeType>>;

    template<class Target, class T, class>
    friend Target handle_cast(const NodeHandle<T>&);

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

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(std::shared_ptr<NodeType> node) : m_node(std::move(node)) {}
    NodeHandle(std::weak_ptr<NodeType> node) : m_node(std::move(node)) {}

    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    NodeHandle(std::shared_ptr<T> node) : m_node(std::static_pointer_cast<NodeType>(std::move(node))) {}

    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    NodeHandle(NodeHandle<T> node) : m_node(std::move(node.m_node)) {}
    /// @}

    /// Destructor.
    ~NodeHandle() noexcept override {
        // Locking this mutex seems like an unnecessary precaution and in fact I think it is ... however, clang
        // thread sanitizer does report A LOT of data races when multiple handles from different threads are destroyed
        // around the same time (which happens on every application shutdown).
        // So we protect the deallocation with a mutex. Should this ever be a performance concern just remove it as it
        // really shouldn't affect the correctness of the program, but until then there seems to be no other way to
        // ensure that the thread sanitizer doesn't produce an avalanche of false positives on each run.
        NOTF_GUARD(std::lock_guard(s_mutex));
        m_node.reset();
    }

    /// Implicit conversion to an (untyped) NodeHandle.
    operator AnyNodeHandle() { return AnyNodeHandle(std::static_pointer_cast<AnyNode>(m_node.lock())); }

    // identification ---------------------------------------------------------
public:
    /// @{
    /// Checks whether the NodeHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_node.expired(); }
    bool is_valid() const { return !is_expired(); }
    explicit operator bool() const { return is_valid(); }
    /// @}

    /// Uuid of this Node.
    auto get_uuid() const { return _get_node()->get_uuid(); }

    /// The Graph-unique name of this Node.
    std::string get_name() const { return _get_node()->get_name(); }

    // TODO: `get_uuid` and `get_name` shouldn't be methods on the handle, see TODO below

    // access -----------------------------------------------------------------
public:
    Interface* operator->() { return _get_interface(); }
    const Interface* operator->() const { return _get_interface(); }

    // comparison -------------------------------------------------------------
public:
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
    Interface* _get_interface() {
        if (!this_thread::is_the_ui_thread()) {
            // TODO: letting the NodeHandle -> operator throw if it's not the UI thread makes rendering difficult...
            //      is there a better way to do this? Ideally, I'd think, the methods itself would make sure that it's
            //      the UI thread, if it is really that important
            //      Actually, all const methods should be fine without check while all mutable must be called from the
            //      ui thread
            // NOTF_THROW(ThreadError, "NodeHandles may only be modified from the UI thread");
        }
        return reinterpret_cast<Interface*>(_get_node().get());
    }
    const Interface* _get_interface() const { return reinterpret_cast<Interface*>(_get_node().get()); }

    /// Allows each NodeHandle subclass to steal the Node of any other Handle.
    /// @param handle   Handle to steal from, becomes invalid after this method returned.
    template<class T>
    static std::weak_ptr<T> _steal_node(NodeHandle<T>&& handle) {
        return std::move(handle.m_node);
    }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// The handled Node, non owning.
    std::weak_ptr<NodeType> m_node;
};

/// Equality comparison with another NodeHandle.
template<class LeftType, class RightType>
bool operator==(const NodeHandle<LeftType>& lhs, const NodeHandle<RightType>& rhs) noexcept {
    return is_weak_ptr_equal(lhs.m_node, rhs.m_node);
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

template<class NodeType>
struct std::hash<notf::NodeHandle<NodeType>> {
    constexpr size_t operator()(const notf::NodeHandle<NodeType>& handle) const noexcept {
        return notf::hash_mix(notf::to_number(handle._get_ptr()));
    }
};
