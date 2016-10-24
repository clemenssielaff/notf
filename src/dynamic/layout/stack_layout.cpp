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

void StackLayout::update_claim()
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
    m_claim = new_claim;
}

void StackLayout::relayout(const Size2r size)
{
    // TODO: CONTINUE HERE
}

} // namespace notf

