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

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Returns the name of a Node.
    static const std::string& _name(valid_ptr<const Node*> node);
};

} // namespace detail

// ----------------------------------------------------------------------------

template<class T>
struct NodeHandle : public detail::NodeHandleBase {
    static_assert(std::is_base_of<Node, T>::value, "The type wrapped by NodeHandle<T> must be a subclass of Node");

    friend class access::_NodeHandle;

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
    /// @throws Node::no_node_error    If the given Node is empty or of the wrong type.
    NodeHandle(const std::shared_ptr<T>& node) : m_node(node)
    {
        { // test if the given node is of the correct type
            if (!dynamic_cast<T*>(node.get())) {
                notf_throw(no_node_error, "Cannot wrap Node \"{}\" into Handle of wrong type", node->name());
            }
        }
    }
    NodeHandle(std::weak_ptr<T> node) : m_node(std::move(node))
    {

        { // test if the given node is alive and of the correct type
            auto raw_node = m_node.lock();
            if (!raw_node) {
                notf_throw(no_node_error, "Cannot create Handle for empty node");
            }
            if (!dynamic_cast<T*>(raw_node.get())) {
                notf_throw(no_node_error, "Cannot wrap Node \"{}\" into Handle of wrong type", _name(raw_node.get()));
            }
        }
    }
    template<class U, typename = std::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(const std::shared_ptr<U>& other) : NodeHandle(std::dynamic_pointer_cast<T>(other))
    {}
    template<class U, typename = std::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(std::weak_ptr<U>&& other) : NodeHandle(std::dynamic_pointer_cast<T>(other.lock()))
    {}
    template<class U, typename = std::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(U* other) : NodeHandle(std::dynamic_pointer_cast<T>(other->shared_from_this()))
    {}
    /// @}

    /// Move constructor.
    /// @param other    NodeHandle to move from.
    NodeHandle(NodeHandle&& other) : m_node(std::move(other.m_node)) { other.m_node.reset(); }

    /// Move constructor for NodeHandles of any base class of T.
    /// The resulting NodeHandle will be invalid, if the other handle cannot be dynamic-cast to T.
    /// @param other    NodeHandle to move from.
    template<class U, typename = std::enable_if_t<std::is_base_of<U, T>::value>>
    NodeHandle(NodeHandle<U>&& other) : m_node(std::move(other.m_node))
    {
        other.m_node.reset();
    }

    /// Move assignment operator.
    /// @param other    NodeHandle to move from.
    NodeHandle& operator=(NodeHandle&& other)
    {
        m_node = std::move(other.m_node);
        other.m_node.reset();
        return *this;
    }

    /// Move-Assignment operator for NodeHandles of any base class of T.
    /// The resulting NodeHandle will be invalid, if the other handle cannot be dynamic-cast to T.
    /// @param other    NodeHande to move-assign from.
    template<class U, typename = std::enable_if_t<std::is_base_of<U, T>::value>>
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

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Handled Node.
    NodeWeakPtr m_node;
};

// accessors -------------------------------------------------------------------------------------------------------- //

#ifdef NOTF_TEST
struct TestNode;
#endif

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
