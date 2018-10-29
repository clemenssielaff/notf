#pragma once

#include "notf/app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// run time widget ================================================================================================== //

class RunTimeWidget : public AnyWidget {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Widget.
    RunTimeWidget(valid_ptr<Node*> parent) : AnyWidget(parent) {}
};

NOTF_CLOSE_NAMESPACE
