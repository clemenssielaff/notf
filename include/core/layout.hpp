#pragma once

#include <type_traits>

#include "core/layout_item.hpp"

namespace notf {

/**
 * @brief Abstract Layout baseclass.
 */
class Layout : public LayoutItem {

    friend class LayoutItem;

public: // methods
    /// @brief Virtual Destructor
    virtual ~Layout() override;

    /// @brief Tests if a given LayoutItem is a child of this LayoutItem.
    bool has_child(const std::shared_ptr<LayoutItem>& candidate) const;

    /// @brief Returns the number of items in this Layout.
    size_t count_children() const { return _get_children().size(); }

    /// @brief Checks if this Layout is empty.
    bool is_empty() const { return m_children.empty(); }

    /// @brief Shows (if possible) or hides this Layout.
    void set_visible(const bool is_visible) { _set_visible(is_visible); }

public: // signals
    /// @brief Emitted when a new child LayoutItem was added to this one.
    /// @param Handle of the new child.
    Signal<Handle> child_added;

    /// @brief Emitted when a child LayoutItem of this one was removed.
    /// @param Handle of the removed child.
    Signal<Handle> child_removed;

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Application-unique Handle of this Item.
    explicit Layout(const Handle handle)
        : LayoutItem(handle)
    {
    }

    /// @brief Returns a child LayoutItem, is invalid if no child with the given Handle exists.
    std::shared_ptr<LayoutItem> _get_child(const Handle child_handle) const;

    /// @brief Returns all children of this LayoutItem.
    const std::unordered_map<Handle, std::shared_ptr<LayoutItem>>& _get_children() const { return m_children; }

    /// @brief Adds the given child to this LayoutItem.
    void _add_child(std::shared_ptr<LayoutItem> child_object);

    /// @brief Removes the child with the given Handle.
    void _remove_child(const Handle child_handle);

    virtual void _cascade_visibility(const VISIBILITY visibility) override;

    virtual void _redraw() override;

    /*
     * @brief Tells this LayoutItem to update its Claim based on the combined Claims of its children.
     *
     * Layouts and Widgets need to "negotiate" the Layout.
     * Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
     * If its Claim changes, its respective parent might need to update as well - up to the first Layout that does not
     * update its Claim (at the latest, the LayoutRoot never updates its Claim).
     */
    virtual void _update_claim() = 0;

    virtual void _relayout(const Size2r size) override = 0;

private: // fields
    /// @brief All children of this Layout.
    std::unordered_map<Handle, std::shared_ptr<LayoutItem>> m_children;
};

} // namespace notf
