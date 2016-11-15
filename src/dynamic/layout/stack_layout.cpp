#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/transform2.hpp"
#include "common/vector_utils.hpp"
#include "core/widget.hpp"

namespace notf {

void StackLayout::add_item(std::shared_ptr<LayoutItem> item)
{
    // if the item is already child of this Layout, place it at the end
    if (has_child(item)) {
        remove_one_unordered(m_items, item->get_handle());
    }
    else {
        _add_child(item);
    }
    m_items.emplace_back(std::move(item->get_handle()));
    _update_parent_layout();
}

std::shared_ptr<Widget> StackLayout::get_widget_at(const Vector2& /*local_pos*/)
{
    return {};
}

void StackLayout::_update_claim()
{
    // construct the new claim
    Claim new_claim;
    if ((m_direction == STACK_DIRECTION::LEFT_TO_RIGHT) || (m_direction == STACK_DIRECTION::RIGHT_TO_LEFT)) { // horizontal
        for (Handle handle : m_items) {
            new_claim.add_horizontal(_get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_horizontal().add_offset((m_items.size() - 1) * m_spacing);
        }
    }
    else {
        assert((m_direction == STACK_DIRECTION::TOP_TO_BOTTOM) || (m_direction == STACK_DIRECTION::BOTTOM_TO_TOP)); // vertical
        for (Handle handle : m_items) {
            new_claim.add_vertical(_get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_vertical().add_offset((m_items.size() - 1) * m_spacing);
        }
    }
    _set_claim(new_claim);
}

void StackLayout::_remove_item(const Handle item_handle)
{
    auto it = std::find(m_items.begin(), m_items.end(), item_handle);
    if (it == m_items.end()) {
        log_critical << "Failed to remove unknown item " << item_handle << " from StackLayout " << get_handle();
        return;
    }
    m_items.erase(it);
}

void StackLayout::_relayout(const Size2r size)
{
    const size_t item_count = m_items.size();
    if (item_count == 0) {
        return;
    }
    if ((m_direction == STACK_DIRECTION::LEFT_TO_RIGHT) || (m_direction == STACK_DIRECTION::RIGHT_TO_LEFT)) { // horizontal
        const float width_per_item = size.width / item_count;
        const Size2r item_size{width_per_item, size.height};
        float x_offset = 0.f;
        for (const Handle handle : m_items) {
            auto child = _get_child(handle);
            _update_item(child, item_size, Transform2::translation({x_offset, 0}));
            x_offset += width_per_item;
        }
    }
    else {
        const float height_per_item = size.height / item_count;
        const Size2r item_size{size.width, height_per_item};
        float y_offset = 0.f;
        for (const Handle handle : m_items) {
            auto child = _get_child(handle);
            _update_item(child, item_size, Transform2::translation({0, y_offset}));
            y_offset += height_per_item;
        }
    }
}

} // namespace notf
