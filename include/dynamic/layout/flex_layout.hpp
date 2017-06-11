#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

/**********************************************************************************************************************/

/** The FlexLayout class offers a way to arrange items in one or multiple rows or columns.
 * It behaves similar to the CSS Flex Box Layout.
 */
class FlexLayout : public Layout {
public: // types ******************************************************************************************************/
    /** Direction in which items in a Layout can be stacked. */
    enum class Direction : unsigned char {
        LEFT_TO_RIGHT,
        TOP_TO_BOTTOM,
        RIGHT_TO_LEFT,
        BOTTOM_TO_TOP,
    };

    /** Alignment of items in a Layout along the main and cross axis. */
    enum class Alignment : unsigned char {
        START,         // items stacked towards the start of the parent, no additional spacing
        END,           // items stacked towards the end of the parent, no additional spacing
        CENTER,        // items centered in parent, no additional spacing
        SPACE_BETWEEN, // equal spacing between items, no spacing between items and border
        SPACE_AROUND,  // single spacing between items and border, double spacing between items
        SPACE_EQUAL,   // equal spacing between the items and the border
    };

    /** How a Layout wraps. */
    enum class Wrap : unsigned char {
        NO_WRAP, // no wrap
        WRAP,    // wraps towards the lower-right corner
        REVERSE, // wraps towards the upper-left corner
    };

    /** Constructor.
     * @param direction Direction of the stack.
     */
    PROTECTED_EXCEPT_FOR_BINDINGS FlexLayout(const Direction direction);

public: // methods ****************************************************************************************************/
    /** Factory.
     * @param direction Direction of the stack.
     */
    static std::shared_ptr<FlexLayout> create(const Direction direction = Direction::LEFT_TO_RIGHT);

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

    /** Defines the direction in which the FlexLayout is stacked. */
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

    /** Adds a new Item into the Layout.
     * @param item  Item to place at the end of the Layout. If the item is already a child, it is moved to the end.
     */
    void add_item(ItemPtr item);

private: // methods ***************************************************************************************************/
    /** Performs the layout of a single stack.
     * @param stack         Items in the stack.
     * @param grant         Size of the stack.
     * @param main_offset   Start offset of the first item in the main axis.
     * @param cross_offset  Start offset of the first item in the cross axis.
     * @param new_size      (out) New size of the Layout.
     */
    void _layout_stack(const std::vector<ScreenItem*>& stack, const Size2f grant,
                       const float main_offset, const float cross_offset, Size2f& new_size);

    virtual void _remove_child(const Item* child_item) override;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;

private: // fields
    /** Direction in which the FlexLayout is stacked. */
    Direction m_direction;

    /** Alignment of items in the main direction. */
    Alignment m_main_alignment;

    /** Alignment of items in the cross direction. */
    Alignment m_cross_alignment;

    /** Cross alignment the entire content if the Layout wraps. */
    Alignment m_content_alignment;

    /** How items in the Layout are wrapped. */
    Wrap m_wrap;

    /** Padding around the Layout's borders. */
    Padding m_padding;

    /** Spacing between items in the Layout in the main direction. */
    float m_spacing;

    /** Spacing between stacks, if this Layout wraps. */
    float m_cross_spacing;
};

} // namespace notf
