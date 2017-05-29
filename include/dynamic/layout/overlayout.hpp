#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

/**********************************************************************************************************************/

/** The Overlayout stacks all of its children on top of each other.
 *
 */
class Overlayout : public Layout {
public: // types ******************************************************************************************************/
    /** Horizontal alignment of all items in the Layout. */
    enum class AlignHorizontal : unsigned char {
        LEFT,
        CENTER,
        RIGHT,
    };

    /** Vertical alignment of all items in the Layout. */
    enum class AlignVertical : unsigned char {
        TOP,
        CENTER,
        BOTTOM,
    };

    /** Constructor. */
    PROTECTED_EXCEPT_FOR_BINDINGS
    Overlayout();

public: // methods ****************************************************************************************************/
    /** Factory. */
    static std::shared_ptr<Overlayout> create();

    /** Horizontal alignment of all items in the Layout. */
    AlignHorizontal get_horizontal_alignment() const { return m_horizontal_alignment; }

    /** Vertical alignment of all items in the Layout. */
    AlignVertical get_vertical_alignment() const { return m_vertical_alignment; }

    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

    /** Defines the alignment of each Item in the Layout. */
    void set_alignment(const AlignHorizontal horizontal, const AlignVertical vertical)
    {
        m_horizontal_alignment = std::move(horizontal);
        m_vertical_alignment   = std::move(vertical);
        _relayout();
    }

    /** Defines the padding around the Layout's border.
     * @throw   std::runtime_error if the padding is invalid.
     */
    void set_padding(const Padding& padding);

    /** Adds a new Item into the Layout.
     * @param item     Item to place at the front end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(ItemPtr item);

private: // methods ***************************************************************************************************/
    virtual void _remove_child(const Item* child_item) override;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Aabrf _get_children_aabr() const override;

    virtual Claim _aggregate_claim() override;

    virtual void _relayout() override;

private: // fields ****************************************************************************************************/
    /** Horizontal alignment of all items in the Layout. */
    AlignHorizontal m_horizontal_alignment;

    /** Vertical alignment of all items in the Layout. */
    AlignVertical m_vertical_alignment;

    /** Padding around the Layout's borders. */
    Padding m_padding;
};

} // namespace notf
