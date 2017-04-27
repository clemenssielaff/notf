#pragma once

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

/** Abstract Layout baseclass. */
class Layout : public ScreenItem {

    friend class Item;

protected: // constructor  ********************************************************************************************/
    /** Default Constructor. */
    Layout();

public: // methods ****************************************************************************************************/
    /** (Un-)Sets an override Claim for this Layout.
     * Layouts with an override Claim do not dynamically aggregate one from their children.
     * To unset an override Claim, pass a zero Claim.
     * @param claim New override Claim.
     * @return True iff the Claim was modified.
     */
    bool set_claim(const Claim claim);

    /** Returns an iterator that goes over all Items in this Layout in order from back to front. */
    virtual std::unique_ptr<LayoutIterator> iter_items() const = 0;

public: // signals  ***************************************************************************************************/
    /** Emitted when a new child Item was added to this one.
     * @param ItemID of the new child.
     */
    Signal<ItemID> child_added;

    /** Emitted when a child Item of this one was removed.
     * @param ItemID of the removed child.
     */
    Signal<ItemID> child_removed;

    virtual bool _set_size(const Size2f size) override;

    /** Removes a single Item from this Layout.
     * Does nothing, if the Item is not a child of this Layout.
     */
    virtual void remove_item(const ItemPtr& item) = 0;

    /** Updates the Claim of this Layout.
     * @return  True, iff the Claim was modified.
     */
    bool _update_claim();

    /** Tells this Layout to create a new Claim for itself from the combined Claims of all of its children.
     * @returns The aggregated Claim.
     */
    virtual Claim _aggregate_claim() = 0;

    /** Updates the layout of Items in this Layout.
     * Call this function when something about the Layout changed that could cause one or more Items to update.
     */
    virtual void _relayout() = 0;

protected: // static methods ******************************************************************************************/
    /** Allows any Layout subclass to update another Item's parent. */
    static void _set_item_parent(Item* item, ItemPtr parent) { item->_set_parent(parent); }

    /** Allows any Layout subclass to call `_set_size` on any other ScreenItem. */
    static bool _set_item_size(ScreenItem* item, const Size2f size) { return item->_set_size(std::move(size)); }

private: // members ***************************************************************************************************/
    /** If true, this Layout provides its own Claim and does not aggregate it from its children. */
    bool m_override_claim;
};

} // namespace notf
