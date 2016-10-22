#pragma once

#include <type_traits>

#include "core/layout_object.hpp"

namespace notf {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * \brief Abstract Layout baseclass.
 * Only reason for existence is to provide a common baseclass for all Layouts.
 * Otherwise it is a normal LayoutObject.
 */
class AbstractLayout : public LayoutObject {

public: // methods
    /// \brief Virtual Destructor
    virtual ~AbstractLayout() override;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit AbstractLayout(const Handle handle)
        : LayoutObject(handle)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * \brief Layout specialization template.
 */
template <typename LayoutItemType, typename = std::enable_if_t<std::is_base_of<LayoutItem, LayoutItemType>::value>>
class BaseLayout : public AbstractLayout {

public: // methods
    /// \brief Shows (if possible) or hides this Layout.
    using LayoutObject::set_visible;

    /// \brief Returns a requested LayoutItem by its LayoutObject's Handle.
    /// The returned LayoutItem is invalid, if this Layout has no child LayoutObject with the given Handle.
    const LayoutItemType& get_item(const Handle handle)
    {
        static const LayoutItemType INVALID;
        const std::unique_ptr<LayoutItem>& item = get_child(handle);
        if (item) {
            return static_cast<LayoutItemType&>(*item.get());
        }
        return INVALID;
    }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit BaseLayout(const Handle handle)
        : AbstractLayout(handle)
    {
    }

    /// \brief Adds the given child to this LayoutObject.
    void add_item(std::unique_ptr<LayoutItemType> child_item)
    {
        add_child(std::move(child_item));
    }

private: // methods
    // hide a few of your parent's methods from subclasses
    using LayoutObject::get_child;
    using LayoutObject::add_child;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace notf
