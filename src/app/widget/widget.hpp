#pragma once

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Widget : public Node {

    /// Constructor.
    Widget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent, const std::string& name = {})
        : Node(token, scene, parent)
    {
        if (!name.empty()) {
            set_name(name);
        }
    }

    /// Destructor.
    ~Widget() override;
};

NOTF_CLOSE_NAMESPACE
