#pragma once

#include "notf/app/graph/node.hpp"

NOTF_OPEN_NAMESPACE

// root node ======================================================================================================== //

class RootNode : public Node<> {

    friend Accessor<RootNode, detail::Graph>;
    friend Accessor<RootNode, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(RootNode);

    /// Only Windows are allowed as children of the root.
#ifndef NOTF_TEST
    using allowed_child_types = std::tuple<Window>;
#endif

    /// Only the Root Node itself may be its own parent.
    using allowed_parent_types = std::tuple<RootNode>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    RootNode() : Node<>(this) {}

    /// Removes all children from underneath the Root Node.
    void clear_children() { _clear_children(); }

private:
    /// Finalizes this RootNode.
    void _finalize_root() { AnyNode::AccessFor<RootNode>::set_finalized(*this); }

    /// Adds a new Window as child of this RootNode.
    void _add_window(WindowPtr window);

    /// Deletes a Window as child of this RootNode
    void _remove_window(const Window* window);
};

// root node accessors ============================================================================================== //

template<>
class Accessor<RootNode, detail::Graph> {
    friend detail::Graph;

    /// Finalizes the given RootNode.
    static void finalize_root(RootNode& node) { node._finalize_root(); }
};

template<>
class Accessor<RootNode, Window> {
    friend Window;

    /// Adds a new Window as child of the given RootNode.
    static void add_window(RootNode& node, WindowPtr window) { node._add_window(std::move(window)); }
};

NOTF_CLOSE_NAMESPACE
