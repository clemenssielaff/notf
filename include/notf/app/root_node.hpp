#pragma once

#include "./window.hpp"
#include "./node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

class Scene {};

struct Pol{
    using properties = std::tuple<>;
};

class RootNode : public CompileTimeNode<Pol> {

public:
    /// Only allow Windows as children of the root.
    using allowed_child_types = std::tuple<Window>;

    // methods --------------------------------------------------------------------------------- //
protected:
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    RootNode() : CompileTimeNode<Pol>(this) {}

    void make_foo()
    {
        NodeMasterHandle hand = _create_child<Window>(this);
        Node* nod = const_cast<Node*>(hand.m_id);
//        _create_child<Window>(nod);
    }
};

NOTF_CLOSE_NAMESPACE
