#pragma once

#include <type_traits>

#include "core/layout_item.hpp"

namespace notf {

/**
 * @brief Abstract Layout baseclass.
 */
class Layout : public LayoutItem {

    friend class LayoutItem;

public: // enums
    /** Direction in which items in a Layout can be stacked. */
    enum class Direction : unsigned char {
        LEFT_TO_RIGHT,
        TOP_TO_BOTTOM,
        RIGHT_TO_LEFT,
        BOTTOM_TO_TOP,
    };

    /** Alignment of items in a Layout along the main and cross axis. */
    enum class Alignment : unsigned char {
        START,
        END,
        CENTER,
        SPACE_BETWEEN,
        SPACE_AROUND,
    };

    /** How a Layout wraps. */
    enum class Wrap : unsigned char {
        NONE,
        WRAP,
        REVERSE,
    };

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

    /** Layout-specific removal a of child item.
     * When a child is removed from the Layout, it calls _remove_child(), which takes care of the changes in the
     * LayoutItem hierarchy.
     * However, most Layouts have an additional data structure for sorted, easy access to their children and it is
     * this methods's job to remove the child from there.
     */
    virtual void _remove_item(const Handle item_handle) = 0;

    virtual void _relayout(const Size2f size) override = 0;

private: // fields
    /// @brief All children of this Layout.
    std::unordered_map<Handle, std::shared_ptr<LayoutItem>> m_children;
};

} // namespace notf
