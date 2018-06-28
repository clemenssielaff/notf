#pragma once

#include "app/node.hpp"
#include "app/widget/claim.hpp"
#include "common/aabr.hpp"
#include "common/matrix3.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Widget : public Node {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    Widget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent, const std::string& name = {})
        : Node(token, scene, parent)
    {
        // name the widget
        if (!name.empty()) {
            set_name(name);
        }
    }

    /// Destructor.
    ~Widget() override;

protected:
    virtual void _paint() = 0;
};

NOTF_CLOSE_NAMESPACE
