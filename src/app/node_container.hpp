#pragma once

#include <map>

#include "app/forwards.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _NodeContainer;
} // namespace access

// ================================================================================================================== //

/// Container for all child nodes of a Node.
class NodeContainer {

    friend class access::_NodeContainer<Node>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_NodeContainer<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Tests if this container is empty.
    bool empty() const noexcept { return m_order.empty(); }

    /// Number of Nodes in the container.
    size_t size() const noexcept { return m_order.size(); }

    /// Checks if the container contains a Node by the given name.
    /// @param name Name to check for.
    bool contains(const std::string& name) const { return m_names.count(name); }

    /// Checks if the container contains a given Node.
    /// @param node Node to check for
    bool contains(const Node* node) const
    {
        for (const auto& child : m_order) {
            if (child.raw().get() == node) {
                return true;
            }
        }
        return false;
    }

    /// Requests an non-owning pointer to a child Node in this container by name.
    /// @param name     Name of the requested child Node.
    /// @returns        Non-owning pointer to the reqested Node.
    ///                 Is empty if no node with the given name exists.
    NodeWeakPtr get(const std::string& name) const
    {
        auto it = m_names.find(name);
        if (it == m_names.end()) {
            return {};
        }
        return it->second;
    }

    /// Adds a new Node to the container.
    /// @param node Node to add.
    /// @returns    True iff the node was inserted successfully, false otherwise.
    bool add(valid_ptr<NodePtr> node);

    /// Erases a given Node from the container.
    void erase(const NodePtr& node);

    /// Clears all Nodes from the container.
    void clear()
    {
        m_order.clear();
        m_names.clear();
    }

    /// Moves the given node in front of all of its siblings.
    /// @param node     Node to move.
    void stack_front(const valid_ptr<Node*> node);

    /// Moves the given node behind all of its siblings.
    /// @param node     Node to move.
    void stack_back(const valid_ptr<Node*> node);

    /// Moves the node at a given index before a given sibling.
    /// @param index    Index of the node to move
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error     If the sibling is not a sibling of this node.
    void stack_before(const size_t index, const valid_ptr<Node*> relative);

    /// Moves the node at a given index behind a given sibling.
    /// @param index    Index of the node to move
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error     If the sibling is not a sibling of this node.
    void stack_behind(const size_t index, const valid_ptr<Node*> sibling);

    /// @{
    /// Reference to the Node in the back of the stack.
    auto& back() { return m_order.front(); }
    const auto& back() const { return m_order.front(); }
    /// @}

    /// @{
    /// Reference to the Node in the front of the stack.
    auto& front() { return m_order.back(); }
    const auto& front() const { return m_order.back(); }
    /// @}

    /// @{
    /// Iterator-adapters for traversing the contained Nodes in order.
    auto begin() noexcept { return m_order.begin(); }
    auto begin() const noexcept { return m_order.cbegin(); }
    auto cbegin() const noexcept { return m_order.cbegin(); }
    auto end() noexcept { return m_order.end(); }
    auto end() const noexcept { return m_order.cend(); }
    auto cend() const noexcept { return m_order.cend(); }
    /// @}

    /// @{
    /// Index-based access to a Node in the container.
    auto& operator[](size_t pos) { return m_order[pos]; }
    const auto& operator[](size_t pos) const { return m_order[pos]; }
    /// @}

private:
    /// Updates the name of one of the child nodes.
    /// This function DOES NOT UPDATE THE NAME OF THE NODE itself, just the name by which the parent knows it.
    /// @param node         Node to rename, must be part of the container and still have its old name.
    /// @param new_name     New name of the node.
    void _rename(valid_ptr<const Node*> node, std::string new_name);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All Nodes in order, also provides ownership.
    std::vector<valid_ptr<NodePtr>> m_order;

    /// Provides name-based lookup of the contained Nodes.
    std::map<std::string, NodeWeakPtr> m_names;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_NodeContainer<Node> {
    friend class notf::Node;

    /// Updates the name of one of the child nodes.
    /// This function DOES NOT UPDATE THE NAME OF THE NODE itself, just the name by which the parent knows it.
    /// @param node         Node to rename, must be part of the container and still have its old name.
    /// @param new_name     New name of the node.
    static void rename_child(NodeContainer& container, valid_ptr<const Node*> node, std::string name)
    {
        container._rename(node, std::move(name));
    }

    /// Direct access to the NodeContainers map of names to Nodes.
    static const std::map<std::string, NodeWeakPtr>& name_map(const NodeContainer& container)
    {
        return container.m_names;
    }
};

NOTF_CLOSE_NAMESPACE
