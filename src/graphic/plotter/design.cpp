#include "notf/graphic/plotter/design.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

void PlotterDesign::complete() {
    // TODO: optimize plotter design after it has been completed
    //  1. Drop intermediary Commands (like multiple set_xform-Commands, but is true for all).
    //  2. Remove all stroke-related Commands after the last `stroke` Command was issued (same for fill and write).
}

NOTF_CLOSE_NAMESPACE
