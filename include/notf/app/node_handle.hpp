#pragma once

#include "notf/meta/pointer.hpp"

#include "./app.hpp"

NOTF_OPEN_NAMESPACE

// node handle ====================================================================================================== //

class NodeHandle {

    friend class Node;
    friend struct std::hash<NodeHandle>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default (empty) Constructor.
    NodeHandle() = default;

    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(const NodePtr& node) : m_node(node), m_id(node.get()) {}

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
    const Uuid& get_uuid() const;

    /// The Node-unique name of this Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::string_view get_name() const;

    /// Comparison with another NodeHandle.
    bool operator==(const NodeHandle& rhs) const noexcept { return m_id == rhs.m_id; }
    bool operator!=(const NodeHandle& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator with another NodeHandle.
    bool operator<(const NodeHandle& rhs) const noexcept { return m_id < rhs.m_id; }

    /// Comparison with a NodePtr.
    friend bool operator==(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return lhs.m_id == rhs.get(); }
    friend bool operator!=(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator==(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return lhs.get() == rhs.m_id; }
    friend bool operator!=(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return !(lhs == rhs); }

    /// Less-than operator with a NodePtr.
    friend bool operator<(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return lhs.m_id < rhs.get(); }
    friend bool operator<(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return lhs.get() < rhs.m_id; }

private:
    /// Locks and returns an owning pointer to the handled Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    NodePtr _get_node() const
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// The handled Node, non owning.
    NodeWeakPtr m_node;

    /// Identifier of this Node Handle.
    /// Is to be used for identification only!
    const Node* m_id = nullptr;
};

NOTF_CLOSE_NAMESPACE

/// Hash
namespace std {
template<>
struct hash<notf::NodeHandle> {
    size_t operator()(const notf::NodeHandle& handle) const noexcept
    {
        return notf::hash_mix(notf::to_number(handle.m_id));
    }
};
} // namespace std
