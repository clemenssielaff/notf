#pragma once

#include "./node_compiletime.hpp"
#include "./node_runtime.hpp"

NOTF_OPEN_NAMESPACE

// any root node ==================================================================================================== //

class AnyRootNode {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Only Windows are allowed as children of the root.
    using allowed_child_types = std::tuple<Window>;

    /// Only the Root Node itself may be its own parent.
    using allowed_parent_types = std::tuple<AnyRootNode>;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Default Constructor.
    AnyRootNode() = default;

public:
    /// Virtual Destructor.
    virtual ~AnyRootNode() = default;

    /// Finalizes this Node.
    virtual void finalize() = 0;

protected:
    template<class T>
    void _finalize_node(T& node)
    {
        Node::AccessFor<AnyRootNode>::finalize(node);
    }
};

// run time root node =============================================================================================== //

class RunTimeRootNode : public RunTimeNode, public AnyRootNode {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    RunTimeRootNode() : RunTimeNode(this), AnyRootNode() {}

    /// Finalizes this Node.
    void finalize() final { _finalize_node(*this); }
};

// compile time root node =========================================================================================== //

template<class Properties>
class CompileTimeRootNode : public CompileTimeNode<Properties>, public AnyRootNode {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    CompileTimeRootNode() : CompileTimeNode<Properties>(this), AnyRootNode() {}

    /// Finalizes this Node.
    void finalize() final { _finalize_node(*this); }
};

NOTF_CLOSE_NAMESPACE
