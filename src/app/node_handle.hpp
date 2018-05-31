#pragma once

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
class _NodeHandle;
} // namespace access

// ================================================================================================================== //

template<class T>
struct NodeHandle {
    static_assert(std::is_base_of<Node, T>::value, "The type wrapped by NodeHandle<T> must be a subclass of Node");

    friend class access::_NodeHandle;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    using Access = access::_NodeHandle;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// @{
    /// Constructor.
    /// @param node     Handled Node.
    /// @throws Node::no_node_error    If the given Node is empty or of the wrong type.
    NodeHandle(const std::shared_ptr<T>& node) : m_node(node)
    {
        { // test if the given node is of the correct type
            if (!dynamic_cast<T*>(node.get())) {
                notf_throw_format(Node::no_node_error, "Cannot wrap Node \"{}\" into Handler of wrong type",
                                  node->name());
            }
        }
    }
    NodeHandle(std::weak_ptr<T> node) : m_node(std::move(node))
    {

        { // test if the given node is alive and of the correct type
            NodePtr raw_node = m_node.lock();
            if (!raw_node) {
                notf_throw(Node::no_node_error, "Cannot create Handler for empty node");
            }
            if (!dynamic_cast<T*>(raw_node.get())) {
                notf_throw_format(Node::no_node_error, "Cannot wrap Node \"{}\" into Handler of wrong type",
                                  raw_node->name());
            }
        }
    }
    /// @}

public:
    NOTF_NO_COPY_OR_ASSIGN(NodeHandle);

    /// Default constructor.
    NodeHandle() = default;

    /// Move constructor.
    /// @param other    NodeHandle to move from.
    NodeHandle(NodeHandle&& other) : m_node(std::move(other.m_node)) { other.m_node.reset(); }

    /// Move assignment operator.
    /// @param other    NodeHandle to move from.
    NodeHandle& operator=(NodeHandle&& other)
    {
        m_node = std::move(other.m_node);
        other.m_node.reset();
        return *this;
    }

    /// @{
    /// The managed BaseNode instance correctly typed.
    /// @throws Node::no_node_error    If the handled Node has been deleted.
    T* operator->()
    {
        NodePtr raw_node = m_node.lock();
        if (!raw_node) {
            notf_throw(Node::no_node_error, "Node has been deleted");
        }
        return static_cast<T*>(raw_node.get());
    }
    const T* operator->() const { return const_cast<NodeHandle<T>*>(this)->operator->(); }
    /// @}

    /// Checks if the Handle is currently valid.
    bool is_valid() const { return !m_node.expired(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Handled Node.
    NodeWeakPtr m_node;
};

// accessors -------------------------------------------------------------------------------------------------------- //

class access::_NodeHandle {
    friend class notf::Node;

    /// Extracts a shared_ptr from a NodeHandle.
    template<class T>
    static risky_ptr<NodePtr> get(const NodeHandle<T>& handle)
    {
        return handle.m_node.lock();
    }

    /// @{
    /// Factory
    /// @param node     Handled Node.
    /// @throws Node::no_node_error    If the given Node is empty or of the wrong type.
    template<class T>
    static NodeHandle<T> create(const std::shared_ptr<T>& node)
    {
        return NodeHandle<T>(node);
    }
    template<class T>
    static NodeHandle<T> create(std::weak_ptr<T>&& node)
    {
        return NodeHandle<T>(std::forward<T>(node));
    }
    /// @}
};

NOTF_CLOSE_NAMESPACE
