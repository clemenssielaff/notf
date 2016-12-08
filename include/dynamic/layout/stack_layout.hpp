#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"

#include "utils/private_except_for_bindings.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

class StackLayout;

/**********************************************************************************************************************/

/**
 * @brief StackLayout Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class StackLayoutIterator : public LayoutIterator {

    friend class MakeSmartEnabler<StackLayoutIterator>;

private: // for MakeSmartEnabler<StackLayoutIterator>;
    explicit StackLayoutIterator(const StackLayout* stack_layout)
        : m_layout(stack_layout)
        , m_index(0)
    {
    }

public: // methods
    virtual ~StackLayoutIterator() = default;

    /** Advances the Iterator one step, returns the next LayoutItem or nullptr if the iteration has finished. */
    virtual const Item* next() override;

private: // fields
    /** StackLayout that is iterated over. */
    const StackLayout* m_layout;

    /** Index of the next LayoutItem to return. */
    size_t m_index;
};

/**********************************************************************************************************************/

/**
 * @brief The StackLayout class
 */
class StackLayout : public Layout {

    friend class MakeSmartEnabler<StackLayout>;
    friend class StackLayoutIterator;

public: // methods
    /** Direction in which items are stacked. */
    Direction get_direction() const { return m_direction; }

    /** Alignment of items in the main direction. */
    Alignment get_alignment() const { return m_main_alignment; }

    /** Alignment of items in the cross direction. */
    Alignment get_cross_alignment() const { return m_cross_alignment; }

    /** Cross alignment the entire content if the Layout wraps. */
    Alignment get_content_alignment() const { return m_content_alignment; }

    /** How (and if) overflowing lines are wrapped. */
    Wrap get_wrap() const { return m_wrap; }

    /** True if overflowing lines are wrapped. */
    bool is_wrapping() const { return m_wrap != Wrap::NO_WRAP; }

    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

    /** Spacing between items. */
    float get_spacing() const { return m_spacing; }

    /** Spacing between stacks of items if this Layout is wrapped. */
    float get_cross_spacing() const { return m_cross_spacing; }

    /** Defines the direction in which the StackLayout is stacked. */
    void set_direction(const Direction direction);

    /** Defines the alignment of stack items in the main direction. */
    void set_alignment(const Alignment alignment);

    /** Defines the alignment of stack items in the cross direction. */
    void set_cross_alignment(const Alignment alignment);

    /** Defines the cross alignment the entire content if the Layout wraps. */
    void set_content_alignment(const Alignment alignment);

    /** Defines how (and if) overflowing lines are wrapped. */
    void set_wrap(const Wrap wrap);

    /** Defines the padding around the Layout's border. */
    void set_padding(const Padding& padding);

    /** Defines the spacing between items. */
    void set_spacing(float spacing);

    /** Defines the spacing between stacks of items if this Layout is wrapped. */
    void set_cross_spacing(float spacing);

    /** Adds a new LayoutItem into the Layout.
     * @param item  Item to place at the end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(std::shared_ptr<Item> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    // clang-format off
private_except_for_bindings: // methods for MakeSmartEnabler<StackLayout>
    /** @param direction Direction of the stack. */
    explicit StackLayout(const Direction direction)
        : Layout()
        , m_direction(direction)
        , m_main_alignment(Alignment::START)
        , m_cross_alignment(Alignment::START)
        , m_content_alignment(Alignment::START)
        , m_wrap(Wrap::NO_WRAP)
        , m_padding(Padding::none())
        , m_spacing(0.f)
        , m_cross_spacing(0.f)
        , m_items()
    {
    }

    // clang-format on
protected: // methods
    virtual bool _update_claim() override;

    virtual void _remove_item(const Item* item) override;

    virtual void _relayout() override;

private: // methods
    /** Performs the layout of a single stack.
     * @param stack         Items in the stack.
     * @param total_size    Size of the stack.
     * @param main_offset   Start offset of the first item in the main axis.
     * @param cross_offset  Start offset of the first item in the cross axis.
     */
    void _layout_stack(const std::vector<Item*>& stack, const Size2f total_size, const float main_offset, const float cross_offset);

private: // fields
    /** Direction in which the StackLayout is stacked. */
    Direction m_direction;

    /** Alignment of items in the main direction. */
    Alignment m_main_alignment;

    /** Alignment of items in the cross direction. */
    Alignment m_cross_alignment;

    /** Cross alignment the entire content if the Layout wraps. */
    Alignment m_content_alignment;

    /** How items in the Layout are wrapped. */
    Wrap m_wrap; // TODO: reverse-wrap does nothing

    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** Spacing between items in the Layout in the main direction. */
    float m_spacing;

    /** Spacing between stacks, if this Layout wraps. */
    float m_cross_spacing;

    /** All items in this Layout in order. */
    std::vector<Item*> m_items;
};

} // namespace notf
