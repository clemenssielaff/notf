#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"

#include "utils/binding_accessors.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

class Overlayout;

/**********************************************************************************************************************/

/**
 * @brief Overlayout Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class OverlayoutIterator : public LayoutIterator {

    friend class MakeSmartEnabler<OverlayoutIterator>;

private: // for MakeSmartEnabler<OverlayoutIterator>;
    OverlayoutIterator(const Overlayout* overlayout)
        : m_layout(overlayout)
        , m_index(0)
    {
    }

public: // methods
    virtual ~OverlayoutIterator() = default;

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
    virtual const Item* next() override;

private: // fields
    /** Overlayout that is iterated over. */
    const Overlayout* m_layout;

    /** Index of the next Item to return. */
    size_t m_index;
};

/**********************************************************************************************************************/

/**
 * @brief The Overlayout class
 */
class Overlayout : public Layout {

    friend class MakeSmartEnabler<Overlayout>;
    friend class OverlayoutIterator;

public: // methods
    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

    /** Defines the padding around the Layout's border. */
    void set_padding(const Padding& padding);

    /** Sets an explicit Claim for this Layout.
     * The default Claim makes use of all available space but doesn't take any as long as there are others more greedy
     * that itself.
     */
    void set_claim(Claim claim) { _set_claim(std::move(claim)); }

    /** Adds a new Item into the Layout.
     * @param item     Item to place at the front end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(std::shared_ptr<Item> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    // clang-format off
protected_except_for_bindings: // methods for MakeSmartEnabler<Overlayout>
    Overlayout()
        : Layout()
        , m_padding(Padding::none())
        , m_items()
    {
    }

    // clang-format on
protected: // methods
    virtual bool _update_claim() override { return false; } // the Overlayout brings its own Claim to the table

    virtual void _remove_item(const Item* item) override;

    virtual void _relayout() override;

private: // fields
    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** All items in this Layout in order. */
    std::vector<Item*> m_items;
};

} // namespace notf
