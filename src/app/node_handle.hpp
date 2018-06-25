#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
class _NodeHandle;
} // namespace access

// ================================================================================================================== //

namespace detail {

struct NodeHandleBase {

    /// Exception thrown when the Node has gone out of scope (Scene must have been deleted).
    NOTF_EXCEPTION_TYPE(no_node_error);
};

} // namespace detail

// ----------------------------------------------------------------------------

template<class T>
struct NodeHandle : public detail::NodeHandleBase {
    static_assert(std::is_base_of<Node, T>::value, "The type wrapped by NodeHandle<T> must be a subclass of Node");

    friend class access::_NodeHandle;

    template<class>
    friend struct NodeHandle; // befriend all other NodeHandle<> implementations

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    using Access = access::_NodeHandle;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(NodeHandle);

    /// Empty default constructor.
    NodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param node     Handled Node.
    NodeHandle(std::shared_ptr<T> node) : m_node(std::move(node)) {}
    NodeHandle(std::weak_ptr<T> node) : m_node(std::move(node)) {}
    template<class U, typename = notf::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(const std::shared_ptr<U>& node) : NodeHandle(std::dynamic_pointer_cast<T>(node))
    {}
    template<class U, typename = notf::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(std::weak_ptr<U>&& node) : NodeHandle(std::dynamic_pointer_cast<T>(node.lock()))
    {}
    template<class U, typename = notf::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(U* node) : NodeHandle(std::dynamic_pointer_cast<T>(node->shared_from_this()))
    {}
    /// @}

    /// Move constructor.
    /// @param other    NodeHandle to move from.
    NodeHandle(NodeHandle&& other) noexcept : m_node(std::move(other.m_node)) { other.m_node.reset(); }

    /// Move constructor for NodeHandles of any base class of T.
    /// The resulting NodeHandle will be invalid, if the other handle cannot be dynamic-cast to T.
    /// @param other    NodeHandle to move from.
    template<class U, typename = notf::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(NodeHandle<U>&& other) : m_node(std::move(other.m_node))
    {
        other.m_node.reset();
    }

    /// Move assignment operator.
    /// @param other    NodeHandle to move from.
    NodeHandle& operator=(NodeHandle&& other) noexcept
    {
        m_node = std::move(other.m_node);
        other.m_node.reset();
        return *this;
    }

    /// Move-Assignment operator for NodeHandles of any base class of T.
    /// The resulting NodeHandle will be invalid, if the other handle cannot be dynamic-cast to T.
    /// @param other    NodeHande to move-assign from.
    template<class U, typename = notf::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle& operator=(NodeHandle<U>&& other)
    {
        m_node = std::dynamic_pointer_cast<T>(std::move(other.m_node).lock());
        other.m_node.reset();
    }

    /// Checks if the Handle is currently valid.
    bool is_valid() const { return !m_node.expired(); }

    /// The managed BaseNode instance correctly typed.
    /// @throws Node::no_node_error    If the handled Node has been deleted.
    operator T*()
    {
        NodePtr raw_node = m_node.lock();
        if (!raw_node) {
            notf_throw(no_node_error, "Node has been deleted");
        }
        return static_cast<T*>(raw_node.get());
    }
    T* operator->() { return operator T*(); }

    /// @{
    /// Comparison operator with another NodeHandle of arbitrary type.
    /// Tests the identity of both handles, meaning two expired Handles will still be different.
    /// Two empty handles are equal, but one empty / one expired are not.
    /// @param other    NodeHandle to compare to.
    template<class U>
    bool operator==(const NodeHandle<U>& other) const
    {
        return weak_ptr_equal(m_node, other.m_node);
    }
    template<class U>
    bool operator!=(const NodeHandle<U>& other) const
    {
        return !operator==(other);
    }
    /// @}

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Handled Node.
    NodeWeakPtr m_node;
};

// accessors -------------------------------------------------------------------------------------------------------- //

class access::_NodeHandle {
    friend class notf::Node;
#ifdef NOTF_TEST
    friend struct notf::TestNode;
#endif

    /// Extracts a shared_ptr from a NodeHandle.
    template<class T>
    static risky_ptr<NodePtr> get(const NodeHandle<T>& handle)
    {
        return handle.m_node.lock();
    }
};

NOTF_CLOSE_NAMESPACE
