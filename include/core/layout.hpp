#pragma once

#include "common/claim.hpp"
#include "core/layout_item.hpp"

namespace notf {

/**********************************************************************************************************************/

/**
 * @brief Abstact Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class LayoutIterator {

protected: // methods
    LayoutIterator() = default;

public: // methods
    virtual ~LayoutIterator() = default;

    /** Advances the Iterator one step, returns the next LayoutItem or nullptr if the iteration has finished. */
    virtual const LayoutItem* next() = 0;
};

/**********************************************************************************************************************/

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
    virtual ~Layout() override;

    /** Tests if a given LayoutItem is a child of this LayoutItem. */
    bool has_item(const std::shared_ptr<LayoutItem>& candidate) const;

    /** Returns the number of items in this Layout. */
    size_t get_item_count() const { return _get_children().size(); }

    /** Checks if this Layout is empty. */
    bool is_empty() const { return m_children.empty(); }

    /** Returns an iterator that goes over all items in this Layout in order from back to front. */
    virtual std::unique_ptr<LayoutIterator> iter_items() const = 0;

    virtual const Claim& get_claim() const override { return m_claim; }

    /** Shows (if possible) or hides this Layout. */
    void set_visible(const bool is_visible) { _set_visible(is_visible); }

public: // signals
    /** Emitted when a new child LayoutItem was added to this one.
     * @param Handle of the new child.
     */
    Signal<Handle> child_added;

    /** Emitted when a child LayoutItem of this one was removed.
     * @param Handle of the removed child.
     */
    Signal<Handle> child_removed;

protected: // methods
    /** @param handle   Application-unique Handle of this Item. */
    explicit Layout(const Handle handle)
        : LayoutItem(handle)
        , m_claim()
    {
    }

    /** Returns all children of this LayoutItem. */
    const std::vector<std::shared_ptr<LayoutItem>>& _get_children() const { return m_children; }

    /** Adds the given child to this LayoutItem. */
    void _add_child(std::shared_ptr<LayoutItem> item);

    /** Removes the given child LayoutItem. */
    void _remove_child(const LayoutItem* item);

    virtual void _cascade_visibility(const VISIBILITY visibility) override;

    /** Updates the Claim but does not trigger any layouting. */
    bool _set_claim(const Claim claim)
    {
        if (claim != m_claim) {
            m_claim = std::move(claim);
            return true;
        }
        return false;
    }

    virtual bool _set_size(const Size2f size) override;

    /** Tells this LayoutItem to update its Claim based on the combined Claims of its children.
     *
     * Layouts and Widgets need to "negotiate" the Layout.
     * Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
     * If its Claim changes, its respective parent might need to update as well - up to the first Layout that does not
     * update its Claim (at the latest, the LayoutRoot never updates its Claim).
     */
    virtual bool _update_claim() = 0;

    /** Updates the layout of items in this Layout. */
    virtual void _relayout() = 0;

    /** Layout-specific removal a of child item.
     * When a child is removed from the Layout, it calls _remove_child(), which takes care of the changes in the
     * LayoutItem hierarchy.
     * However, most Layouts have an additional data structure for sorted, easy access to their children and it is
     * this methods's job to remove the child from there.
     */
    virtual void _remove_item(const LayoutItem* item) = 0;

private: // fields
    /** All child items contained in this Layout. */
    std::vector<std::shared_ptr<LayoutItem>> m_children;

    /// @brief The Claim of a LayoutItem determines how much space it receives in the parent Layout.
    Claim m_claim;
};

} // namespace notf
