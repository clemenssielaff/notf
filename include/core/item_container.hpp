#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace notf {

class Item;
using ItemPtr = std::shared_ptr<Item>;

namespace detail {

/**********************************************************************************************************************/

/** Abstract Item Container.
 * Used by Item subclasses to contain child Items.
 */
struct ItemContainer {
    /** Destructor. */
    virtual ~ItemContainer();

    /** Clears all Items from this Container */
    void clear();

    /** Applies a function to all Items in this Container. */
    virtual void apply(std::function<void(Item*)> function);

    /** Applies a function to all Items in this Container and then recursively down their child hierarchy.
     * The function is first applied to parents because children are aware of their parents but generally not the other
     * way around.
     */
    void apply_recursive(std::function<void(Item*)> function);
};

/**********************************************************************************************************************/

/** Widgets have no child Items and use this empty Container as a placeholder instead. */
struct EmptyItemContainer : public ItemContainer {
    virtual void apply(std::function<void(Item*)>) override {}
};

/**********************************************************************************************************************/

/** Controller (and some Layouts) have a single child Item. */
struct SingleItemContainer : public ItemContainer {
    virtual void apply(std::function<void(Item*)> function) override;

private: // fields ****************************************************************************************************/
    /** The singular Item contained in this Container. */
    ItemPtr m_item;
};

/**********************************************************************************************************************/

/** Many Layouts keep their child Items in a list. */
struct ItemList : public ItemContainer {
    virtual void apply(std::function<void(Item*)> function) override;

private: // fields ****************************************************************************************************/
    /** All Items contained in the list. */
    std::vector<ItemPtr> m_items;
};

} // namespace detail
} // namespace notf
