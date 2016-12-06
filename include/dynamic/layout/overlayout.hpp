#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"

#include "utils/protected_except_for_bindings.hpp"

namespace notf {

class Overlayout;

/**********************************************************************************************************************/

/**
 * @brief Overlayout Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class OverlayoutIterator : public LayoutIterator {

    friend class StackLayout;

protected: // for LayoutRoot
    OverlayoutIterator(const Overlayout* overlayout)
        : m_layout(overlayout)
        , m_index(0)
    {
    }

public: // methods
    virtual ~OverlayoutIterator() = default;

    /** Advances the Iterator one step, returns the next LayoutItem or nullptr if the iteration has finished. */
    virtual const LayoutItem* next() override;

private: // fields
    /** Overlayout that is iterated over. */
    const Overlayout* m_layout;

    /** Index of the next LayoutItem to return. */
    size_t m_index;
};

/**********************************************************************************************************************/

/**
 * @brief The Overlayout class
 */
class Overlayout : public Layout {

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

    /** Adds a new LayoutItem into the Layout.
     * @param item     Item to place at the front end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(std::shared_ptr<LayoutItem> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    protected_except_for_bindings : // methods
                                    explicit Overlayout()
        : Layout()
        , m_padding(Padding::none())
        , m_items()
    {
    }

protected: // methods
    virtual bool _update_claim() override { return false; } // the Overlayout brings its own Claim to the table

    virtual void _remove_item(const LayoutItem* item) override;

    virtual void _relayout() override;

private: // fields
    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** All items in this Layout in order. */
    std::vector<LayoutItem*> m_items;
};

} // namespace notf
