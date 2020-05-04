#include "notf/graphic/plotter/design.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

void PlotterDesign::_complete() {
    // TODO: optimize plotter design after it has been completed
    //  1. Drop intermediary Commands (like multiple set_xform-Commands, but is true for all).
    //  2. Remove all stroke-related Commands after the last `stroke` Command was issued (same for fill and write).
    //  3. Remove double stroke/fill/writes without change in xform (only if fully opaque)

    m_is_dirty.store(false, std::memory_order_release);
}

NOTF_CLOSE_NAMESPACE
