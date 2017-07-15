#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/fwds.hpp"

namespace notf {

namespace detail {

/**********************************************************************************************************************/

/** Abstract Item Container.
 * Used by Item subclasses to contain child Items.
 */
struct ItemContainer {

    friend class notf::Item;

    /** Destructor. */
    virtual ~ItemContainer();

    /** Clears all Items from this Container */
    virtual void clear() = 0;

    /** Applies a function to all Items in this Container. */
    virtual void apply(std::function<void(Item*)> function) = 0;

    /** Checks whether this Container contains a given Item. */
    virtual bool contains(const Item* item) const = 0;

    /** Checks whether this Container is empty or not. */
    virtual bool is_empty() const = 0;

protected: // methods
    /** Sets the parent of all Items to nullptr without evoking proper reparenting.
     * Is only used by the Item destructor.
     */
    void destroy();
};

/**********************************************************************************************************************/

/** Widgets have no child Items and use this empty Container as a placeholder instead. */
struct EmptyItemContainer : public ItemContainer {
    virtual ~EmptyItemContainer() override;

    virtual void clear() override {}

    virtual void apply(std::function<void(Item*)>) override {}

    virtual bool contains(const Item*) const override { return false; }

    virtual bool is_empty() const override { return true; }
};

/**********************************************************************************************************************/

/** Controller (and some Layouts) have a single child Item. */
struct SingleItemContainer : public ItemContainer {
    virtual ~SingleItemContainer() override;

    virtual void clear() override;

    virtual void apply(std::function<void(Item*)> function) override;

    virtual bool contains(const Item* child) const override { return child == item.get(); }

    virtual bool is_empty() const override { return static_cast<bool>(item); }

    /** The singular Item contained in this Container. */
    ItemPtr item;
};

/**********************************************************************************************************************/

/** Many Layouts keep their child Items in a list. */
struct ItemList : public ItemContainer {
    virtual ~ItemList() override;

    virtual void clear() override;

    virtual void apply(std::function<void(Item*)> function) override;

    virtual bool contains(const Item* child) const override;

    virtual bool is_empty() const override { return items.empty(); }

    /** All Items contained in the list. */
    std::vector<ItemPtr> items;
};

} // namespace detail
} // namespace notf
