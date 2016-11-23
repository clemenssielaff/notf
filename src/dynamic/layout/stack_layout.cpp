#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>
#include <map>
#include <set>

#include "common/claim.hpp"
#include "common/float_utils.hpp"
#include "common/log.hpp"
#include "common/transform2.hpp"
#include "common/vector_utils.hpp"
#include "core/widget.hpp"

namespace { // anonymous

using namespace notf;

/**
 * Helper struct used to abstract away which claim-direction and stack-direction is used for layouting.
 */
struct ItemAdapter {
    float upper_bound;
    float preferred;
    float scale_factor;
    float result;
};

float distribute_surplus(float surplus, std::map<int, std::set<ItemAdapter*>>& batches)
{
    for (auto batch_it = batches.rbegin(); batch_it != batches.rend(); ++batch_it) {
        std::set<ItemAdapter*>& batch = batch_it->second;
        bool on_first_phase = true;
        while (!batch.empty()) {

            // do nothing, if the surplus has been depleted
            if (approx(surplus, 0.f)) {
                batch.clear();
                continue;
            }

            // first, supply space to all items that are smaller than their preferred size
            if (on_first_phase) {
                on_first_phase = false;
                float total_deficit = 0.f;
                float total_scale_factor = 0.f;
                for (auto it = batch.begin(); it != batch.end();) {
                    ItemAdapter* item = *it;
                    float item_deficit = item->preferred - item->result;
                    assert(item_deficit >= 0);
                    if (item_deficit > 0) {
                        total_deficit += item_deficit;
                        total_scale_factor += item->scale_factor;
                        assert(item->scale_factor > 0);
                    }
                    else if (approx(item->upper_bound - item->result, 0.f)) {
                        it = batch.erase(it);
                        continue;
                    }
                    ++it;
                }
                if (total_deficit > 0.f) {
                    total_deficit = min(total_deficit, surplus);
                    assert(total_scale_factor > 0);
                    const float deficit_per_scale_factor = total_deficit / total_scale_factor;
                    for (ItemAdapter* item : batch) {
                        const float item_deficit = item->preferred - item->result;
                        if (item_deficit > 0.f) {
                            if (item_deficit < deficit_per_scale_factor * item->scale_factor) {
                                surplus -= item->preferred - item->result;
                                item->result = item->preferred;
                                on_first_phase = true;
                            }
                        }
                    }
                    if (!on_first_phase) {
                        for (auto it = batch.begin(); it != batch.end();) {
                            ItemAdapter* item = *it;
                            if (item->preferred - item->result > 0.f) {
                                item->result += deficit_per_scale_factor * item->scale_factor;
                            }
                            if (approx(item->result, item->upper_bound)) {
                                it = batch.erase(it);
                            }
                            else {
                                ++it;
                            }
                        }
                        surplus -= total_deficit;
                    }
                }
            }

            // in the second phase, distribute the remaining space
            else {
                float total_scale_factor = 0.f;
                for (ItemAdapter* item : batch) {
                    assert(item->upper_bound - item->result > 0);
                    assert(item->scale_factor > 0);
                    total_scale_factor += item->scale_factor;
                }
                float surplus_per_scale_factor = surplus / total_scale_factor;
                bool surplus_fits_in_batch = true;
                for (auto it = batch.begin(); it != batch.end();) {
                    ItemAdapter* item = *it;
                    if (item->result + (item->scale_factor * surplus_per_scale_factor) > item->upper_bound) {
                        surplus_fits_in_batch = false;
                        surplus -= item->upper_bound - item->result;
                        item->result = item->upper_bound;
                        it = batch.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                if (surplus_fits_in_batch) {
                    for (ItemAdapter* item : batch) {
                        item->result += item->scale_factor * surplus_per_scale_factor;
                    }
                    batch.clear();
                    surplus = 0.f;
                }
            }
        }
    }
    return surplus;
}

} // namespace anonymous

namespace notf {

void StackLayout::set_direction(const Direction direction)
{
    if (m_direction == direction) {
        return;
    }
    m_direction = direction;
    _update_claim();
    if (_is_dirty()) {
        _update_parent_layout();
    }
}

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

void StackLayout::set_alignment(const Alignment alignment)
{
    if (alignment == m_main_alignment) {
        return;
    }
    m_main_alignment = alignment;
    _relayout(get_size());
}

void StackLayout::set_cross_alignment(const Alignment alignment)
{
    if (alignment == m_cross_alignment) {
        return;
    }
    m_cross_alignment = alignment;
    _relayout(get_size());
}

void StackLayout::set_wrapping(const bool wrap)
{
    if (wrap == m_is_wrapping) {
        return;
    }
    m_is_wrapping = wrap;
    _relayout(get_size());
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
    if ((m_direction == Direction::LEFT_TO_RIGHT) || (m_direction == Direction::RIGHT_TO_LEFT)) { // horizontal
        for (Handle handle : m_items) {
            new_claim.add_horizontal(_get_child(handle)->get_claim());
        }
        if (!m_items.empty()) {
            new_claim.get_horizontal().add_offset((m_items.size() - 1) * m_spacing);
        }
    }
    else {
        assert((m_direction == Direction::TOP_TO_BOTTOM) || (m_direction == Direction::BOTTOM_TO_TOP)); // vertical
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

void StackLayout::_relayout(const Size2f total_size)
{
    std::vector<std::vector<Handle>> rows;
    if (m_is_wrapping) {
        const bool horizontal = (m_direction == Direction::LEFT_TO_RIGHT) || (m_direction == Direction::RIGHT_TO_LEFT);
        const float available_size = horizontal ? total_size.width : total_size.height;
        std::vector<Handle> current_row;
        float current_size = horizontal ? m_padding.width() : m_padding.height();
        for (Handle item : m_items) {
            const Claim& claim = _get_child(item)->get_claim();
            const Claim::Stretch& stretch = horizontal ? claim.get_horizontal() : claim.get_vertical();
            const float addition = stretch.get_preferred() + m_spacing;
            if (current_size + addition > available_size) {
                rows.emplace_back(std::move(current_row));
                current_row.clear(); // re-using moved container, see http://stackoverflow.com/a/9168917/3444217
                current_size = horizontal ? m_padding.width() : m_padding.height();
            }
            current_size += addition;
            current_row.push_back(item);
        }
        rows.emplace_back(std::move(current_row));
    }
    else {
        rows.push_back(m_items);
    }
    for (const std::vector<Handle>& row : rows) {
        _layout_row(row, total_size);
    }
}

void StackLayout::_layout_row(const std::vector<Handle>& row, const Size2f total_size)
{
    // calculate the actual, availabe size
    const size_t item_count = row.size();
    if (item_count == 0) {
        return;
    }

    const bool horizontal = (m_direction == Direction::LEFT_TO_RIGHT) || (m_direction == Direction::RIGHT_TO_LEFT);
    const float available_width = max(0.f, total_size.width - m_padding.width() - (horizontal ? m_spacing * (item_count - 1) : 0.f));
    const float available_height = max(0.f, total_size.height - m_padding.height() - (horizontal ? 0.f : m_spacing * (item_count - 1)));

    // all elements get at least their minimum size
    float total_min = 0.f;
    std::vector<ItemAdapter> adapters(row.size());
    std::map<int, std::set<ItemAdapter*>> batches;
    for (size_t i = 0; i < row.size(); ++i) {
        const Claim& claim = _get_child(row[i])->get_claim();
        const Claim::Stretch& stretch = horizontal ? claim.get_horizontal() : claim.get_vertical();
        const std::pair<float, float> width_to_height = claim.get_width_to_height();
        if (width_to_height.first > 0.f) {
            if (horizontal) {
                adapters[i].upper_bound = min(stretch.get_max(), available_height * width_to_height.second);
            }
            else {
                adapters[i].upper_bound = min(stretch.get_max(), available_width / width_to_height.first);
            }
        }
        else {
            adapters[i].upper_bound = stretch.get_max();
        }
        adapters[i].preferred = min(adapters[i].upper_bound, stretch.get_preferred());
        adapters[i].scale_factor = stretch.get_scale_factor();
        adapters[i].result = min(adapters[i].upper_bound, stretch.get_min());
        total_min += adapters[i].result;
        batches[stretch.get_priority()].insert(&adapters[i]);
    }

    // layouting is the process by which the surplus space in the layout is distributed
    float surplus = max(0.f, horizontal ? available_width - total_min : available_height - total_min);
    if (surplus > 0) {
        surplus = distribute_surplus(surplus, batches);
    }

    // determine values for alignment along the primary axis
    float alignment_start = 0.f;
    float alignment_spacing = 0.f;
    switch (m_main_alignment) {
    case Alignment::START:
        break;
    case Alignment::END:
        alignment_start = surplus;
        break;
    case Alignment::CENTER:
        alignment_start = surplus * 0.5f;
        break;
    case Alignment::SPACE_BETWEEN:
        alignment_spacing = item_count > 1 ? surplus / static_cast<float>(item_count - 1) : 0.f;
        break;
    case Alignment::SPACE_AROUND:
        alignment_start = surplus / static_cast<float>(item_count * 2);
        alignment_spacing = alignment_start * 2.f;
        break;
    case Alignment::SPACE_EQUAL:
        alignment_start = surplus / static_cast<float>(item_count + 1);
        alignment_spacing = alignment_start;
        break;
    }

    // apply the layout
    float start_offset;
    float step_factor;
    const bool reverse = (m_direction == Direction::RIGHT_TO_LEFT) || (m_direction == Direction::BOTTOM_TO_TOP);
    if (reverse) {
        if (horizontal) {
            start_offset = total_size.width - alignment_start - m_padding.right;
        }
        else {
            start_offset = total_size.height - alignment_start - m_padding.bottom;
        }
        step_factor = -1.f;
    }
    else {
        start_offset = alignment_start + (horizontal ? m_padding.left : m_padding.top);
        step_factor = 1.f;
    }
    float current_offset = start_offset;
    for (size_t index = 0; index < row.size(); ++index) {
        std::shared_ptr<LayoutItem> child = _get_child(row.at(index)); // TODO: this `handle` handling is tedious
        const ItemAdapter& adapter = adapters[index];
        const std::pair<float, float> width_to_height = child->get_claim().get_width_to_height();
        assert(adapter.result >= 0.f);
        assert(adapter.result <= adapter.upper_bound);
        if (horizontal) {
            const Claim::Stretch& vertical = child->get_claim().get_vertical();
            Size2f item_size(adapter.result, min(vertical.get_max(), available_height));
            if (width_to_height.first > 0.f) {
                item_size.height = min(item_size.height, adapter.result / width_to_height.first);
            }
            item_size.height = max(item_size.height, vertical.get_min());
            const float cross_offset = _cross_align_offset(item_size.height, available_height);
            const float applied_offset = reverse ? current_offset - item_size.width : current_offset;
            _update_item(child, item_size, Transform2::translation({applied_offset, m_padding.top + cross_offset}));
        }
        else { // vertical
            const Claim::Stretch& horizontal = child->get_claim().get_horizontal();
            Size2f item_size(min(horizontal.get_max(), available_width), adapter.result);
            if (width_to_height.first > 0.f) {
                item_size.width = min(item_size.width, adapter.result * width_to_height.second);
            }
            item_size.width = max(item_size.width, horizontal.get_min());
            const float cross_offset = _cross_align_offset(item_size.width, available_width);
            const float applied_offset = reverse ? current_offset - item_size.height : current_offset;
            _update_item(child, item_size, Transform2::translation({m_padding.left + cross_offset, applied_offset}));
        }
        current_offset += (adapter.result + m_spacing + alignment_spacing) * step_factor;
    }
}

float StackLayout::_cross_align_offset(const float item_size, const float available_size)
{
    if (available_size > item_size) {
        switch (m_cross_alignment) {
        case Alignment::START:
            return 0;
        case Alignment::END:
            return available_size - item_size;
        case Alignment::CENTER:
        case Alignment::SPACE_BETWEEN:
        case Alignment::SPACE_AROUND:
        case Alignment::SPACE_EQUAL:
            return (available_size - item_size) * 0.5f;
        }
    }
    return 0.f;
}

// TODO: cross overflow is always happending to the south-east ... do I need a separate field for that? In a flex layout, I'd need it anyway...
// make the field a boolean or use a new enum "clockwise" or "counter-clockwise" - those values will work regardless
// what the main direction of the layout is and we won't need to enforce that only orthogonal cross directions can be set
// ... and update them when the main widrection has been reset etc.

} // namespace notf
