#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"

namespace notf {

/**
 * @brief The StackLayout class
 */
class StackLayout : public Layout {

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

    /** Defines the alignment of items in the main direction. */
    void set_alignment(const Alignment alignment);

    /** Defines the alignment of items in the cross direction. */
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

    /// @brief Adds a new LayoutItem into the Layout.
    /// @param item     Item to place at the end of the Layout. If it is already a child, it is moved to last place.
    void add_item(std::shared_ptr<LayoutItem> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

public: // static methods
    /// @brief Factory function to create a new StackLayout.
    /// @param direction    Direction in which the StackLayout is stacked.
    /// @param handle       Handle of this Layout.
    static std::shared_ptr<StackLayout> create(const Direction direction, Handle handle = BAD_HANDLE)
    {
        return _create_object<StackLayout>(handle, direction);
    }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Layout.
    explicit StackLayout(const Handle handle, const Direction direction)
        : Layout(handle)
        , m_direction(direction)
        , m_main_alignment(Alignment::START)
        , m_cross_alignment(Alignment::START)
        , m_content_alignment(Alignment::START)
        , m_wrap(Wrap::NO_WRAP)
        , m_stretch_cross(false)
        , m_padding(Padding::none())
        , m_spacing(0.f)
        , m_cross_spacing(0.f)
        , m_items()
    {
    }

    virtual void _update_claim() override;

    virtual void _remove_item(const Handle item_handle) override;

    virtual void _relayout(const Size2f total_size) override;

private: // methods
    /** Performs the layout of a single stack.
     * @param stack         Items in the stack.
     * @param total_size    Size of the stack.
     * @param main_offset   Start offset of the first item in the main axis.
     * @param cross_offset  Start offset of the first item in the cross axis.
     */
    void _layout_stack(const std::vector<Handle>& stack, const Size2f total_size, const float main_offset, const float cross_offset);

    /** Calculates the cross offset to accomodate the cross alignment constraint. */
    float _cross_align_offset(const float item_size, const float available_size);

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

    /** Defines whether the stacks should stretch in the cross direction or keep to a minimal size. */
    bool m_stretch_cross; // TODO: this cannot be changed yet

    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** Spacing between items in the Layout in the main direction. */
    float m_spacing;

    /** Spacing between stacks, if this Layout wraps. */
    float m_cross_spacing;

    /// @brief All items in this Layout in order.
    std::vector<Handle> m_items;
};

} // namespace notf
