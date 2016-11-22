#include "dynamic/layout/stack_layout.hpp"

#include <assert.h>
#include <map>
#include <set>

#include "common/claim.hpp"
#include "common/float_utils.hpp"
#include "common/log.hpp"
#include "common/set_utils.hpp"
#include "common/transform2.hpp"
#include "common/vector_utils.hpp"
#include "core/widget.hpp"

namespace { // anonymous

using notf::Claim;
using notf::approx;

/**
 * Helper struct used to abstract away which claim-direction and stack-direction is used for layouting.
 */
struct ItemAdapter {

    float upper_bound;

    /** Base size. */
    float preferred;

    /** Scale factor. */
    float scale_factor;

    /** Calculated size. */
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
                for (ItemAdapter* item : batch) {
                    float item_deficit = item->preferred - item->result;
                    assert(item_deficit >= 0);
                    if (item_deficit > 0) {
                        total_deficit += item_deficit;
                        total_scale_factor += item->scale_factor;
                        assert(item->scale_factor > 0);
                    }
                }
                if (total_deficit > 0.f) {
                    total_deficit = notf::min(total_deficit, surplus);
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
                        for (ItemAdapter* item : batch) {
                            if (item->preferred - item->result > 0.f) {
                                item->result += deficit_per_scale_factor * item->scale_factor;
                            }
                        }
                        notf::erase_conditional(batch, [](const ItemAdapter* item) -> bool {
                            return approx(item->result, item->upper_bound);
                        });
                        surplus -= total_deficit;
                    }
                }
            }

            // in the second phase, distribute the remaining space
            else {
                std::vector<ItemAdapter*> finished_items;
                float total_scale_factor = 0.f;
                for (ItemAdapter* item : batch) {
                    assert(item->upper_bound - item->result > 0);
                    assert(item->scale_factor > 0);
                    total_scale_factor += item->scale_factor;
                }
                float surplus_per_scale_factor = surplus / total_scale_factor;
                bool surplus_fits_in_batch = true;
                for (ItemAdapter* item : batch) {
                    if (item->result + (item->scale_factor * surplus_per_scale_factor) > item->upper_bound) {
                        surplus_fits_in_batch = false;
                        surplus -= item->upper_bound - item->result;
                        item->result = item->upper_bound;
                        finished_items.push_back(item);
                    }
                }
                if (surplus_fits_in_batch) {
                    for (ItemAdapter* item : batch) {
                        item->result += item->scale_factor * surplus_per_scale_factor;
                    }
                    batch.clear();
                    surplus = 0.f;
                }
                else {
                    for (ItemAdapter* item : finished_items) {
                        batch.erase(item);
                    }
                }
            }
        }
    }
    return surplus;
}

} // namespace anonymous

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

// TODO: adapt for other than left->right
void StackLayout::_relayout(const Size2f total_size)
{
    // calculate the actual, availabe size
    const size_t item_count = m_items.size();
    if (item_count == 0) {
        return;
    }
    const float available_width = max(0.f, total_size.width - m_padding.left - m_padding.right - (m_spacing * (item_count - 1)));
    const float available_height = max(0.f, total_size.height - m_padding.top - m_padding.bottom);

    // all elements get at least their minimum size
    float total_min = m_padding.left + m_padding.right + (m_spacing * (item_count - 1));
    std::vector<ItemAdapter> adapters(m_items.size());
    std::map<int, std::set<ItemAdapter*>> batches;
    for (size_t i = 0; i < m_items.size(); ++i) {
        const Claim& claim = _get_child(m_items[i])->get_claim();
        const Claim::Direction& direction = claim.get_horizontal();
        {
            const std::pair<float, float> width_to_height = claim.get_width_to_height();
            if (width_to_height.first > 0.f) {
                adapters[i].upper_bound = min(direction.get_max(), available_height / width_to_height.second);
            }
            else {
                adapters[i].upper_bound = direction.get_max();
            }
        }
        adapters[i].preferred = min(adapters[i].upper_bound, direction.get_preferred());
        adapters[i].scale_factor = direction.get_scale_factor();
        adapters[i].result = min(adapters[i].upper_bound, direction.get_min());
        total_min += adapters[i].result;
        batches[direction.get_priority()].insert(&adapters[i]);
    }

    // layouting is the process by which the surplus space in the layout is distributed
    if (available_width > total_min) {
        distribute_surplus(available_width - total_min, batches);
    }

    // apply the layout
    float x_offset = m_padding.left;
    for (size_t i = 0; i < m_items.size(); ++i) {
        std::shared_ptr<LayoutItem> child = _get_child(m_items.at(i)); // TODO: this `handle` handling is tedious
        const ItemAdapter& adapter = adapters[i];

        assert(adapter.result >= 0.f);
        assert(adapter.result <= adapter.upper_bound);

        const std::pair<float, float> width_to_height = child->get_claim().get_width_to_height();
        Size2f item_size(adapter.result, available_height);
        if (width_to_height.first > 0.f) {
            item_size.height = min(available_height, adapter.result / width_to_height.first);
        }
        _update_item(child, item_size, Transform2::translation({x_offset, m_padding.top}));
        x_offset += adapter.result + m_spacing;
    }
}

} // namespace notf
