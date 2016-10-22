#pragma once

#include <type_traits>

#include "core/layout_object.hpp"

namespace notf {

/*
 * \brief Abstract Layout baseclass.
 * Only reason for existence is to provide a common baseclass for all Layouts.
 * Otherwise it is a normal LayoutObject.
 */
class Layout : public LayoutObject {

public: // methods
    /// \brief Virtual Destructor
    virtual ~Layout() override;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit Layout(const Handle handle)
        : LayoutObject(handle)
    {
    }
};

} // namespace notf
