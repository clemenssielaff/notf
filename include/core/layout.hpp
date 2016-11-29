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
        START, // items stacked towards the start of the parent, no additional spacing
        END, // items stacked towards the end of the parent, no additional spacing
        CENTER, // items centered in parent, no additional spacing
        SPACE_BETWEEN, // equal spacing between items, no spacing between items and border
        SPACE_AROUND, // single spacing between items and border, double spacing between items
        SPACE_EQUAL, // equal spacing between the items and the border
    };

    /** How a Layout wraps. */
    enum class Wrap : unsigned char {
        NO_WRAP, // no wrap
        WRAP, // wraps towards the lower-right corner
        WRAP_REVERSE, // wraps towards the upper-left corner
    };

    /** Direction of a cirular motion. */
    enum class Circular : unsigned char {
        CLOCKWISE,
        COUNTERCLOCKWISE,
        CW = CLOCKWISE,
        CCW = COUNTERCLOCKWISE,
        ANTICLOCKWISE = COUNTERCLOCKWISE,
        ACW = COUNTERCLOCKWISE,
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

    /// @brief Returns all children of this LayoutItem.
    const std::vector<std::shared_ptr<LayoutItem>>& _get_children() const { return m_children; }

    /// @brief Adds the given child to this LayoutItem.
    void _add_child(std::shared_ptr<LayoutItem> item);

    /// @brief Removes the given child LayoutItem.
    void _remove_child(const LayoutItem* item);

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
    virtual void _remove_item(const LayoutItem* item) = 0;

    virtual void _relayout(const Size2f size) override = 0;

private: // fields
    /** All child items contained in this Layout. */
    std::vector<std::shared_ptr<LayoutItem>> m_children;
};

} // namespace notf
