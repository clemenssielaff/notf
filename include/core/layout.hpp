#pragma once

#include "common/size2.hpp"
#include "core/item.hpp"

namespace notf {

/**********************************************************************************************************************/

/** Abstact Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class LayoutIterator {

protected: // methods
    /** Default Constructor. */
    LayoutIterator() = default;

public: // methods
    /** Destructor. */
    virtual ~LayoutIterator() = default;

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
    virtual const Item* next() = 0;
};

/**********************************************************************************************************************/

/** Abstract Layout baseclass. */
class Layout : public Item {

    friend class Item;

public: // methods
    virtual ~Layout() override;

    /** Tests if a given Item is a child of this Item. */
    bool has_item(const std::shared_ptr<Item>& candidate) const;

    /** Returns the number of items in this Layout. */
    size_t get_item_count() const { return _get_children().size(); }

    /** Checks if this Layout is empty. */
    bool is_empty() const { return m_children.empty(); }

    /** Returns an iterator that goes over all items in this Layout in order from back to front. */
    virtual std::unique_ptr<LayoutIterator> iter_items() const = 0;

public: // signals
    /** Emitted when a new child Item was added to this one.
     * @param ItemID of the new child.
     */
    Signal<ItemID> child_added;

    /** Emitted when a child Item of this one was removed.
     * @param ItemID of the removed child.
     */
    Signal<ItemID> child_removed;

protected: // methods
    explicit Layout()
        : Item() {}

    /** Returns all children of this Item. */
    const std::vector<std::shared_ptr<Item>>& _get_children() const { return m_children; }

    /** Adds the given child to this Item. */
    void _add_child(std::shared_ptr<Item> item);

    /** Removes the given child Item. */
    void _remove_child(const Item* item);

    virtual bool _set_size(const Size2f& size) override;

    /** Tells this Item to update its Claim based on the combined Claims of its children.
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
     * Item hierarchy.
     * However, most Layouts have an additional data structure for sorted, easy access to their children and it is
     * this methods's job to remove the child from there.
     */
    virtual void _remove_item(const Item* item) = 0;

private: // fields
    /** All child items contained in this Layout. */
    std::vector<std::shared_ptr<Item>> m_children;
};

} // namespace notf
