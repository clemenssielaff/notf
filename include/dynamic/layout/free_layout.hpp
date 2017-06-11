#pragma once

#include "core/layout.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

/**********************************************************************************************************************/

/** The Free Layout contains Items ordered from back to front but does not impose any other attributes upon them.
 * Items are free to move around, scale or rotate.
 * The Free Layout does not aggregate a Claim from its Items, nor does it make any effords to lay them out.
 */
class FreeLayout : public Layout {

    /** Constructor. */
    PROTECTED_EXCEPT_FOR_BINDINGS
    FreeLayout();

public: // methods ****************************************************************************************************/
    /** Factory. */
    static std::shared_ptr<FreeLayout> create();

    /** Adds a new Item into the Layout.5
     * @param item     Item to place at the front end of the Layout. If the item is already a child, it is moved.
     */
    void add_item(ItemPtr item);

private: // methods ***************************************************************************************************/
    virtual void _remove_child(const Item* child_item) override;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;
};

} // namespace notf
