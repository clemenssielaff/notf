#pragma once

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// root node ======================================================================================================== //

class RootNode final : public CompileTimeNode<std::tuple<>> {

    friend Accessor<RootNode, TheGraph>;
    friend Accessor<RootNode, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(RootNode);

    /// Only Windows are allowed as children of the root.
    using allowed_child_types = std::tuple<Window>;

    /// Only the Root Node itself may be its own parent.
    using allowed_parent_types = std::tuple<RootNode>;

private:
    /// Owning list of child Nodes, ordered from back to front.
    using ChildList = Node::AccessFor<RootNode>::ChildList;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    RootNode() : CompileTimeNode<std::tuple<>>(this) {}

private:
    /// Finalizes this RootNode.
    void _finalize() { Node::AccessFor<RootNode>::finalize(*this); }

    /// Adds a new Window as child of this RootNode.
    void _add_window(WindowPtr window);
};

// root node accessors ============================================================================================== //

template<>
class Accessor<RootNode, TheGraph> {
    friend TheGraph;

    /// Finalizes the given RootNode.
    static void finalize(RootNode& node) { node._finalize(); }
};

template<>
class Accessor<RootNode, Window> {
    friend Window;

    /// Adds a new Window as child of the given RootNode.
    static void add_window(RootNode& node, WindowPtr window) { node._add_window(std::move(window)); }
};

NOTF_CLOSE_NAMESPACE
