#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

class Overlayout;

/**********************************************************************************************************************/

/**
 * @brief Overlayout Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class OverlayoutIterator : public LayoutIterator {

public: // methods
    /** Constructor */
    OverlayoutIterator(const Overlayout* overlayout)
        : m_layout(overlayout)
        , m_index(0)
    {
    }

    /** Destructor */
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

    friend class OverlayoutIterator;

public: // enums
    /** Horizontal alignment of all items in the Layout. */
    enum class Horizontal : unsigned char {
        LEFT,
        CENTER,
        RIGHT,
    };

    /** Vertical alignment of all items in the Layout. */
    enum class Vertical : unsigned char {
        TOP,
        CENTER,
        BOTTOM,
    };

private: // factory
    struct make_shared_enabler;

    // clang-format off
protected_except_for_bindings: // factory
    Overlayout();
    // clang-format on

public: // methods
    /** Factory method */
    static std::shared_ptr<Overlayout> create();

    /** Destructor. */
    virtual ~Overlayout() override;

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

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

    /** Defines the padding around the Layout's border.
     * @throw   std::runtime_error if the padding is invalid.
     */
    void set_padding(const Padding& padding);

    /** Horizontal alignment of all items in the Layout. */
    Horizontal get_horizontal_alignment() const { return m_horizontal_alignment; }

    /** Vertical alignment of all items in the Layout. */
    Vertical get_vertical_alignment() const { return m_vertical_alignment; }

    /** Defines the alignment of each Item in the Layout. */
    void set_alignment(const Horizontal horizontal, const Vertical vertical)
    {
        m_horizontal_alignment = std::move(horizontal);
        m_vertical_alignment   = std::move(vertical);
        _relayout();
    }

protected: // methods
    virtual Claim _aggregate_claim() override;

    virtual void _relayout() override;

private: // methods
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

private: // fields
    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** Horizontal alignment of all items in the Layout. */
    Horizontal m_horizontal_alignment;

    /** Vertical alignment of all items in the Layout. */
    Vertical m_vertical_alignment;

    /** All items in this Layout in order. */
    std::vector<ItemPtr> m_items;
};

} // namespace notf
