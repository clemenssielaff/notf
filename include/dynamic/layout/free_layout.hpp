#pragma once

#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

class FreeLayout;

using FreeLayoutPtr = std::shared_ptr<FreeLayout>;

/**********************************************************************************************************************/

/**
 * @brief Free Layout iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class FreeLayoutIterator : public LayoutIterator {

public: // methods
    /** Constructor */
    FreeLayoutIterator(const FreeLayout* overlayout)
        : m_layout(overlayout)
        , m_index(0)
    {
    }

    /** Destructor */
    virtual ~FreeLayoutIterator() = default;

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
    virtual Item* next() override;

private: // fields
    /** Free Layout that is iterated over. */
    const FreeLayout* m_layout;

    /** Index of the next Item to return. */
    size_t m_index;
};

/**********************************************************************************************************************/

/** The Free Layout contains Items ordered from back to front but does not impose any other attributes upon them.
 * Items are free to move around, scale or rotate.
 * The Free Layout does not aggregate a Claim from its Items, nor does it make any effords to lay them out.
 */
class FreeLayout : public Layout {

    friend class FreeLayoutIterator;

public: // static methods
    /** Factory. */
    static FreeLayoutPtr create();

private: // methods
    // clang-format off
protected_except_for_bindings:
    FreeLayout();
    // clang-format on

public: // methods
    /** Destructor. */
    virtual ~FreeLayout() override;

    /** Tests if a given Item is a child of this Item. */
    bool has_item(const ItemPtr& item) const;

    /** Checks if this Layout is empty or not. */
    bool is_empty() const { return m_items.empty(); }

    /** Removes all Items from the Layout. */
    void clear();

    /** Adds a new Item into the Layout.
     * @param item     Item to place at the front end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(ItemPtr item);

    /** Removes a single Item from this Layout.
     * Does nothing, if the Item is not a child of this Layout.
     */
    virtual void remove_item(const ItemPtr& item) override;

    /** Returns the united bounding rect of all Items in the Layout. */
    virtual Aabrf get_content_aabr() const override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

protected: // methods
    virtual Claim _aggregate_claim() override { return {}; }

    virtual void _relayout() override {}

private: // methods
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

private: // fields
    /** All items in this Layout in order. */
    std::vector<ItemPtr> m_items;
};

} // namespace notf
