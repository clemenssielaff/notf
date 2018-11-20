#pragma once

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// root node ======================================================================================================== //

class RootNode final : public CompileTimeNode<std::tuple<>> {

    friend Accessor<RootNode, TheGraph>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(RootNode);

    /// Only Windows are allowed as children of the root.
    using allowed_child_types = std::tuple<Window>;

    /// Only the Root Node itself may be its own parent.
    using allowed_parent_types = std::tuple<RootNode>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    RootNode() : CompileTimeNode<std::tuple<>>(this) {}

private:
    /// Finalizes this RootNode.
    void _finalize() { Node::AccessFor<RootNode>::finalize(*this); }
};

// root node accessors ============================================================================================== //

template<>
class Accessor<RootNode, TheGraph> {
    friend TheGraph;

    /// Finalizes the given RootNode.
    static void finalize(RootNode& node) { node._finalize(); }
};

NOTF_CLOSE_NAMESPACE
