#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/transform2.hpp"
#include "common/vector_utils.hpp"
#include "core/widget.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

void StackLayout::set_spacing(float spacing)
{
    if (spacing < 0.f) {
        log_warning << "Cannot set spacing to less than zero, using zero instead.";
        spacing = 0.f;
    }
    m_spacing = spacing;
    _update_parent_layout();
}

void StackLayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_warning << "Ignoring invalid padding: " << padding;
        return;
    }
    m_padding = padding;
    _update_parent_layout();
}

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
    if ((m_direction == StackDirection::LEFT_TO_RIGHT) || (m_direction == StackDirection::RIGHT_TO_LEFT)) { // horizontal
        for (Handle handle : m_items) {
            new_claim.add_horizontal(_get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_horizontal().add_offset((m_items.size() - 1) * m_spacing);
        }
    }
    else {
        assert((m_direction == StackDirection::TOP_TO_BOTTOM) || (m_direction == StackDirection::BOTTOM_TO_TOP)); // vertical
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

void StackLayout::_relayout(const Size2f size)
{
    const size_t item_count = m_items.size();
    if (item_count == 0) {
        return;
    }

    if ((m_direction == StackDirection::LEFT_TO_RIGHT) || (m_direction == StackDirection::RIGHT_TO_LEFT)) { // horizontal
        const float actual_width = max(0.f, size.width - m_padding.left - m_padding.right - (m_spacing * (item_count - 1)));
        const float actual_height = max(0.f, size.height - m_padding.top - m_padding.bottom);
        const float width_per_item = actual_width / item_count;
        const Size2f item_size{width_per_item, actual_height};
        const float step_size = width_per_item + m_spacing;
        float x_offset = m_padding.left;
        if (m_direction == StackDirection::LEFT_TO_RIGHT) {
            for (const Handle handle : m_items) {
                auto child = _get_child(handle);
                _update_item(child, item_size, Transform2::translation({x_offset, m_padding.top}));
                x_offset += step_size;
            }
        }
        else {
            for (const Handle handle : reverse(m_items)) {
                auto child = _get_child(handle);
                _update_item(child, item_size, Transform2::translation({x_offset, m_padding.top}));
                x_offset += step_size;
            }
        }
    }
    else {
        const float actual_width = max(0.f, size.width - m_padding.left - m_padding.right);
        const float actual_height = max(0.f, size.height - m_padding.top - m_padding.bottom - (m_spacing * (item_count - 1)));
        const float height_per_item = actual_height / item_count;
        const Size2f item_size{actual_width, height_per_item};
        const float step_size = height_per_item + m_spacing;
        float y_offset = m_padding.top;
        if (m_direction == StackDirection::TOP_TO_BOTTOM) {
            for (const Handle handle : m_items) {
                auto child = _get_child(handle);
                _update_item(child, item_size, Transform2::translation({m_padding.left, y_offset}));
                y_offset += step_size;
            }
        }
        else {
            for (const Handle handle : reverse(m_items)) {
                auto child = _get_child(handle);
                _update_item(child, item_size, Transform2::translation({m_padding.left, y_offset}));
                y_offset += step_size;
            }
        }
    }
}

} // namespace notf
