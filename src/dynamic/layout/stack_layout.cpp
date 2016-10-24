#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>

#include "core/widget.hpp"

namespace notf {

void StackLayout::add_item(std::shared_ptr<LayoutItem> /*widget*/)
{
}

std::shared_ptr<Widget> StackLayout::get_widget_at(const Vector2& /*local_pos*/)
{
    return {};
}

bool StackLayout::relayout()
{
    // construct the new claim
    Claim new_claim;
    if ((m_direction == DIRECTION::LEFT_TO_RIGHT) || (m_direction == DIRECTION::RIGHT_TO_LEFT)) { // horizontal
        for (Handle handle : m_items) {
            new_claim.add_horizontal(get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_horizontal().add_offset((m_items.size() - 1) * m_spacing);
        }
    }
    else {
        assert((m_direction == DIRECTION::TOP_TO_BOTTOM) || (m_direction == DIRECTION::BOTTOM_TO_TOP)); // vertical
        for (Handle handle : m_items) {
            new_claim.add_vertical(get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_vertical().add_offset((m_items.size() - 1) * m_spacing);
        }
    }

    /* TODO: CONTINUE HERE
     *
     * There is little use in changing widget size or child layout size right after updating the claim.
     * Instead, update_parent_layout() (or whatever) should be called update_claim().
     * If THAT function returns false, meaning the Claim of parent Layout X did not change,
     * THEN we call X->update_child_layouts.
     */

    set_item_size(get_child(BAD_HANDLE), {0, 0});

    if (new_claim == get_claim()) {
        return false;
    }
    set_claim(new_claim);
    return true;
}

} // namespace notf

