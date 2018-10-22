#pragma once

#include "./node.hpp"
#include "./node_runtime.hpp"

NOTF_OPEN_NAMESPACE

class Window : public RunTimeNode {
    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Window(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
};



NOTF_CLOSE_NAMESPACE
