#pragma once

#include "core/layout_object.hpp"

namespace signal {

/*
 * \brief The base class for all Layout subclasses to distinguish them from Widgets.
 *
 * Doesn't add any functionality to LayoutItem.
 */
class Layout : public LayoutObject {

public: // methods
    /// \brief Virtual Destructor.
    virtual ~Layout() override;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Layout.
    explicit Layout(const Handle handle)
        : LayoutObject(handle)
    {
    }
};

} // namespace signal
