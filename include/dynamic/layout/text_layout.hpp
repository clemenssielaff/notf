#pragma once

#include "common/padding.hpp"
#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

/**********************************************************************************************************************/

/** The TextLayout is used to arrange multiple TextWidgets or other widgets in a continuous text flow.
 *
 */
class TextLayout : public Layout {

    /** Constructor. */
    PROTECTED_EXCEPT_FOR_BINDINGS
    TextLayout();

public: // methods ****************************************************************************************************/
    /** Factory. */
    static std::shared_ptr<TextLayout> create();

    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

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

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;

private: // fields ****************************************************************************************************/
    /** Padding around the Layout's borders. */
    Padding m_padding;
};

} // namespace notf
