#pragma once

#include <type_traits>

#include "core/layout_item.hpp"

namespace notf {

/*
 * \brief Abstract Layout baseclass.
 */
class Layout : public LayoutItem {

public: // methods
    /// \brief Virtual Destructor
    virtual ~Layout() override;

    /// \brief Returns the number of items in this Layout.
    size_t get_count() const { return get_children().size(); }

    /// \brief Checks if this Layout is empty.
    bool is_empty() const { return get_count() == 0; }

    using LayoutItem::set_visible;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit Layout(const Handle handle)
        : LayoutItem(handle)
    {
    }
};

} // namespace notf
