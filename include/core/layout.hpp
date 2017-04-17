#pragma once

#include "common/size2.hpp"
#include "core/screen_item.hpp"

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

/** Abstract Layout baseclass.
 *
 * Size negotiation
 * ================
 * Every ScreenItem has a Claim, that is a minimum / preferred / maximum 2D size that it would like to occupy on the
 * screen.
 * A Layout that has multiple ScreenItems as children, will combine their Claims into a new Claim >= the union of all
 * of its children.
 * This Claim in then used by the parent Layout to distribute its own space to the children.
 * Note that a Claim is just that - a Claim, it is not a hard constraint, even though the Layouts will do their best
 * to accomodate the wishes of each ScreenItem.
 * But there is just so much screen estate to go around and not all of the wishes will be fulfilled.
 * For example, if a StackLayout has 10 Widgets of 100px min width each, but the Window is only 700px wide, the
 * StackLayout's Claim will still have a minimum of 1000px.
 * The WindowLayout will set the StackLayout size to 700px, to which the StackLayout can then react.
 * Either, the 3 Widgets will overflow and not be displayed, or the StackLayout might wrap them into a second row, if
 * the user chooses to set those flags.
 */
class Layout : public ScreenItem {

    friend class Item;

public: // methods
    virtual ~Layout() override;

    /** Tests if a given Item is a child of this Item. */
    bool has_item(const std::shared_ptr<Item>& candidate) const;

    /** Returns the number of items in this Layout. */
    size_t get_item_count() const { return _get_children().size(); }

    /** Checks if this Layout is empty. */
    bool is_empty() const { return m_children.empty(); }

    /** Returns an iterator that goes over all Items in this Layout in order from back to front. */
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
        : ScreenItem() {}

    /** Returns all children of this Layout. */
    const std::vector<std::shared_ptr<Item>>& _get_children() const { return m_children; }

    /** Adds the given Item to this Layout. */
    void _add_child(std::shared_ptr<Item> item);

    /** Removes the given Item Layout. */
    void _remove_child(const Item* item);

    virtual bool _set_size(const Size2f size) override;

    /** Tells this Layout to update its Claim based on the combined Claims of its children.
     *
     * Layouts and Widgets need to "negotiate" the Layout.
     * Whenever a Widget changes its Claim, the parent Layout has to see if it needs to update its Claim accordingly.
     * If its Claim changes, its respective parent might need to update as well - up to the first Layout that does not
     * update its Claim (at the latest, the WindowLayout never updates its Claim).
     */
    virtual bool _update_claim() = 0;

    /** Updates the layout of Items in this Layout. */
    virtual void _relayout() = 0;

    /** Layout-specific removal a of child Item.
     * When a child is removed from the Layout, it calls _remove_child(), which takes care of the changes in the
     * Item hierarchy.
     * However, most Layouts have an additional data structure for sorted, easy access to their children and it is
     * this methods's job to remove the child from there.
     */
    virtual void _remove_item(const Item* item) = 0;

protected: // static methods
    /** Allows any Layout subclass to update another Item's parent. */
    static void _set_item_parent(Item* item, std::shared_ptr<Item> parent) { item->_set_parent(parent); }

    /** Allows any Layout subclass to call `_set_size` on any other ScreenItem. */
    static bool _set_item_size(ScreenItem* item, const Size2f size) { return item->_set_size(std::move(size)); }

    /** Allows any Layout subclass to call `_set_item_transform` on any other ScreenItem. */
    static bool _set_item_transform(ScreenItem* item, const Xform2f transform) { return item->_set_transform(std::move(transform)); }

private: // fields
    /** All child items contained in this Layout. */
    std::vector<std::shared_ptr<Item>> m_children; // TODO: rethink this practice of storing children in Layout and not only in subclasses
};

} // namespace notf
