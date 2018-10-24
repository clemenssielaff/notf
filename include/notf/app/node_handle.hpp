#pragma once

#include "notf/meta/pointer.hpp"

#include "./app.hpp"

NOTF_OPEN_NAMESPACE

// node handle base ================================================================================================= //

namespace detail {

/// Members common to NodeHandle and NodeMasterHandle.
class NodeHandleBase {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    NodeHandleBase() = default;

    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandleBase(const NodePtr& node) : m_node(node), m_id(node.get()) {}

    /// @{
    /// Checks whether the NodeHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_node.expired(); }
    explicit operator bool() const { return !is_expired(); }
    /// @}

    /// The Uuid of this Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    Uuid get_uuid() const;

    /// The Node-unique name of this Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::string_view get_name() const;

    /// Comparison with another NodeHandle.
    bool operator==(const NodeHandleBase& rhs) const noexcept { return m_id == rhs.m_id; }
    bool operator!=(const NodeHandleBase& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator with another NodeHandle.
    bool operator<(const NodeHandleBase& rhs) const noexcept { return m_id < rhs.m_id; }

    /// Comparison with a NodePtr.
    friend bool operator==(const NodeHandleBase& lhs, const NodePtr& rhs) noexcept { return lhs.m_id == rhs.get(); }
    friend bool operator!=(const NodeHandleBase& lhs, const NodePtr& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator==(const NodePtr& lhs, const NodeHandleBase& rhs) noexcept { return lhs.get() == rhs.m_id; }
    friend bool operator!=(const NodePtr& lhs, const NodeHandleBase& rhs) noexcept { return !(lhs == rhs); }

    /// Less-than operator with a NodePtr.
    friend bool operator<(const NodeHandleBase& lhs, const NodePtr& rhs) noexcept { return lhs.m_id < rhs.get(); }
    friend bool operator<(const NodePtr& lhs, const NodeHandleBase& rhs) noexcept { return lhs.get() < rhs.m_id; }

private:
    /// Locks and returns an owning pointer to the handled Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    NodePtr _get_node() const
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// The handled Node, non owning.
    NodeWeakPtr m_node;

    /// Identifier of this Node Handle.
    /// Is to be used for identification only!
    const Node* m_id = nullptr;
};

} // namespace detail

// node handle ====================================================================================================== //

/// Regular Node Handle.
class NodeHandle final : public detail::NodeHandleBase {

    friend struct ::std::hash<NodeHandle>;
    friend class ::notf::Node;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(NodeHandle);

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    NodeHandle() = default;

    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(const NodePtr& node) : detail::NodeHandleBase(node) {}
};

// node master handle =============================================================================================== //

/// Special NodeHandle type that is unique per Node instance and removes the Node when itself goes out of scope.
/// If the Node has already been removed by then, the destructor does nothing.
class NodeMasterHandle final : public detail::NodeHandleBase {

    friend struct ::std::hash<NodeMasterHandle>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(NodeMasterHandle);

    /// Default (empty) Constructor.
    NodeMasterHandle() = default;

    /// Value Constructor.
    /// @param node     Node to handle.
    /// @throws ValueError  If the given NodePtr is empty.
    NodeMasterHandle(NodePtr&& node);

    /// Move assignment operator.
    NodeMasterHandle& operator=(NodeMasterHandle&& other) = default;

    /// Implicit cast to a NodeHandle.
    operator NodeHandle() { return NodeHandle(m_node.lock()); }

    /// Destructor.
    /// Note that the destruction of a Node requires the Graph mutex. Normally (if you store the Handle on the parent
    /// Node or some other Node in the Graph) this does not block, but it might if the mutex is not already held by this
    /// thread.
    ~NodeMasterHandle();
};

// node master handle castable ====================================================================================== //

namespace detail {

/// Type returned by Node::_create_child.
/// Can once (implicitly) be cast to a NodeMasterHandle, but can also be safely ignored without the Node being erased
/// immediately.
class NodeMasterHandleCastable {

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(NodeMasterHandleCastable);

    /// Constructor.
    /// @param node     The newly created Node.
    NodeMasterHandleCastable(NodePtr&& node) : m_node(std::move(node)) {}

    /// Implicit cast to a NodeMasterHandle.
    /// Can be called once.
    /// @throws ValueError  If called more than once or the Node has already expired.
    operator NodeMasterHandle();

    /// Implicit cast to a NodeHandle.
    /// Can be called multiple times.
    operator NodeHandle() { return NodeHandle(m_node.lock()); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The newly created Node.
    /// Is held as a weak_ptr because the user might (foolishly) decide to store this object instead of using for
    /// casting only, and we don't want to keep the Node alive for longer than its parent is.
    NodeWeakPtr m_node;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE

// std::hash specializations ======================================================================================== //

namespace std {

template<>
struct hash<notf::NodeHandle> {
    constexpr size_t operator()(const notf::NodeHandle& handle) const noexcept
    {
        return notf::hash_mix(notf::to_number(handle.m_id));
    }
};

template<>
struct hash<notf::NodeMasterHandle> {
    constexpr size_t operator()(const notf::NodeMasterHandle& handle) const noexcept
    {
        return notf::hash_mix(notf::to_number(handle.m_id));
    }
};

} // namespace std
