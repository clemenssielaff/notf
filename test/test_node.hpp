#pragma once

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Test Accessor providing test-related access functions to a Node.
template<>
class access::_Node<test::Harness> {
public:
    static void unfinalize(Node& node) { Node::s_unfinalized_nodes.insert(&node); }

    static void finalize(Node& node) { node._finalize(); }
};

// ================================================================================================================== //

struct TestNode : public Node {

    /// Constructor.
    TestNode(const FactoryToken& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent) {}

    /// Destructor.
    ~TestNode() override;

    /// Adds a new node as a child of this one.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args>
    NodeHandle<T> add_node(Args&&... args)
    {
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Adds a child node that itself has a given number of children.
    /// @param grandchildren_count  Numer of children in the newly created child node.
    NodeHandle<TestNode> add_subtree(const size_t grandchildren_count)
    {
        NodeHandle<TestNode> child = add_node<TestNode>();
        NodePtr child_ptr = NodeHandle<TestNode>::Access::get(child);
        NOTF_ASSERT(child_ptr);

        // we pretend like the child node creates the grandchildren itself (avoiding additional frozen child copies)
        Node::Access<test::Harness>::unfinalize(*child_ptr);
        for (size_t i = 0; i < grandchildren_count; ++i) {
            child->add_node<TestNode>();
        }
        Node::Access<test::Harness>::finalize(*child_ptr);

        return child;
    }

    /// Removes and existing child from this Node.
    /// @param handle   Handle of the child node to remove.
    template<class T>
    void remove_child(const NodeHandle<T>& handle)
    {
        _remove_child<T>(handle);
    }

    /// Removes all children from this Node.
    void clear() { _clear_children(); }

    /// Reverses the order of all child Nodes.
    void reverse_children();
};

NOTF_CLOSE_NAMESPACE
