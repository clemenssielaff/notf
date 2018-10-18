#pragma once

#include "notf/meta/pointer.hpp"

#include "./app.hpp"

NOTF_OPEN_NAMESPACE

// node handle ====================================================================================================== //

class NodeHandle {

    friend class Node;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default (empty) Constructor.
    NodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(const NodePtr& node) : m_node(node) {}
    NodeHandle(NodeWeakPtr node) : m_node(std::move(node)) {}
    /// @}

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
    bool operator==(const NodeHandle& rhs) const noexcept { return weak_ptr_equal(m_node, rhs.m_node); }
    bool operator!=(const NodeHandle& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator with another NodeHandle.
    bool operator<(const NodeHandle& rhs) const noexcept { return m_node.owner_before(rhs.m_node); }

    //    /// Comparison with a NodePtr.
    //    friend bool operator==(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return lhs.m_node.lock() == rhs;
    //    } friend bool operator!=(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return !(lhs == rhs); } friend
    //    bool operator==(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return lhs == rhs.m_node.lock(); }
    //    friend bool operator!=(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return !(lhs == rhs); }

    //    /// Less-than operator with a NodePtr.
    //    friend bool operator<(const NodeHandle& lhs, const NodePtr& rhs) noexcept { return lhs.m_node.lock() < rhs; }
    //    friend bool operator<(const NodePtr& lhs, const NodeHandle& rhs) noexcept { return lhs < rhs.m_node.lock(); }

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
};

NOTF_CLOSE_NAMESPACE
