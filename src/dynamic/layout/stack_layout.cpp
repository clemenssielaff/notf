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

using notf::Claim;
using notf::approx;

/**
 * Helper struct used to abstract away which claim-direction and stack-direction is used for layouting.
 */
struct ItemAdapter {

    /** Mininum or maximum size (depending on usage). */
    float limit;

    /** Preferred size. */
    float preferred;

    /** Scale factor. */
    float scale_factor;

    /** Calculated size. */
    float result;
};

void distribute_positive_surplus(float surplus, std::map<int, std::set<ItemAdapter*>>& batches)
{
    for (auto batch_it = batches.rbegin(); batch_it != batches.rend(); ++batch_it) {
        std::set<ItemAdapter*>& batch = batch_it->second;
        while (!batch.empty()) {
            if (!approx(surplus, 0.f)) {
                float total_scale_factor = 0.;
                for (const ItemAdapter* item : batch) {
                    assert(item->scale_factor > 0.f);
                    total_scale_factor += item->scale_factor;
                }
                assert(total_scale_factor > 0.f);
                float surplus_per_scale_factor = surplus / total_scale_factor;

                // if the size is higher than the max of any item, it's surplus must be distributed among the batch
                bool surplus_fits_in_batch = true;
                for (const ItemAdapter* item : batch) {
                    if (item->limit < item->preferred + (item->scale_factor * surplus_per_scale_factor)) {
                        surplus -= item->limit;
                        surplus_fits_in_batch = false;
                        auto mutable_item = const_cast<ItemAdapter*>(item);
                        batch.erase(mutable_item);
                        mutable_item->result = item->limit;
                    }
                }

                // if all sizes fit, we can use up the surplus with this batch
                if (surplus_fits_in_batch) {
                    for (const ItemAdapter* item : batch) {
                        auto mutable_item = const_cast<ItemAdapter*>(item);
                        batch.erase(mutable_item);
                        mutable_item->result = item->preferred + (item->scale_factor * surplus_per_scale_factor);
                    }
                    assert(batch.empty());
                    surplus = 0.f;
                }
            }
            else { // no surplus means that all further items are assigned their preferred claim
                for (const ItemAdapter* item : batch) {
                    auto mutable_item = const_cast<ItemAdapter*>(item);
                    batch.erase(mutable_item);
                    mutable_item->result = item->preferred;
                }
                assert(batch.empty());
            }
        }
    }
}

void distribute_negative_surplus(float surplus, std::map<int, std::set<ItemAdapter*>>& batches)
{
    for (auto batch_it = batches.begin(); batch_it != batches.end(); ++batch_it) {
        std::set<ItemAdapter*>& batch = batch_it->second;
        while (!batch.empty()) {
            if (!approx(surplus, 0.f)) {
                float total_scale_factor = 0.;
                for (const ItemAdapter* item : batch) {
                    assert(item->scale_factor > 0.f);
                    total_scale_factor += item->scale_factor;
                }
                assert(total_scale_factor > 0.f);
                float surplus_per_scale_factor = surplus / total_scale_factor;

                // if the size is smaller than the min of any item, it's surplus must be distributed among the batch
                bool surplus_fits_in_batch = true;
                for (const ItemAdapter* item : batch) {
                    if (item->limit > item->preferred + (item->scale_factor * surplus_per_scale_factor)) {
                        surplus += item->limit;
                        surplus_fits_in_batch = false;
                        auto mutable_item = const_cast<ItemAdapter*>(item);
                        batch.erase(mutable_item);
                        mutable_item->result = item->limit;
                    }
                }

                // if all sizes fit, we can use up the surplus with this batch
                if (surplus_fits_in_batch) {
                    for (const ItemAdapter* item : batch) {
                        auto mutable_item = const_cast<ItemAdapter*>(item);
                        batch.erase(mutable_item);
                        mutable_item->result = item->preferred + (item->scale_factor * surplus_per_scale_factor);
                    }
                    assert(batch.empty());
                    surplus = 0.f;
                }
            }
            else { // no surplus means that all further items are assigned their preferred claim
                for (const ItemAdapter* item : batch) {
                    auto mutable_item = const_cast<ItemAdapter*>(item);
                    batch.erase(mutable_item);
                    mutable_item->result = item->preferred;
                }
                assert(batch.empty());
            }
        }
    }
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

void StackLayout::_relayout(const Size2f total_size)
{
    const size_t item_count = m_items.size();
    if (item_count == 0) {
        return;
    }

    // TODO: adapt for other than left->right


    // calculate spacial requirements
    const float available_width = max(0.f, total_size.width - m_padding.left - m_padding.right - (m_spacing * (item_count - 1)));
    const float available_height = max(0.f, total_size.height - m_padding.top - m_padding.bottom);
    float total_preferred = m_padding.left + m_padding.right + (m_spacing * (item_count - 1));
    for (size_t i = 0; i < m_items.size(); ++i) {
        const Claim::Direction& direction = _get_child(m_items[i])->get_claim().get_horizontal();
        total_preferred += direction.get_preferred();
    }
    const float surplus = available_width - total_preferred;

    std::vector<ItemAdapter> adapters(m_items.size());
    std::map<int, std::set<ItemAdapter*>> batches;
    for (size_t i = 0; i < m_items.size(); ++i) {
        const Claim& claim = _get_child(m_items[i])->get_claim();
        const Claim::Direction& direction = claim.get_horizontal();
        total_preferred += direction.get_preferred();
        const std::pair<float, float> width_to_height = claim.get_width_to_height();
        if(surplus >= 0.f){
            if (width_to_height.first > 0.f) {
                adapters[i].limit = min(direction.get_max(), available_height / width_to_height.second);
            } else {
                adapters[i].limit = direction.get_max();
            }
        }
        else {
            if (width_to_height.first > 0.f) {
                adapters[i].limit = min(direction.get_min(), available_height / width_to_height.second);
            } else {
                adapters[i].limit = direction.get_min();
            }
        }
        adapters[i].preferred = direction.get_preferred();
        adapters[i].scale_factor = direction.get_scale_factor();
#ifdef _DEBUG
        adapters[i].result = -1.f;
#endif
        batches[direction.get_priority()].insert(&adapters[i]);
    }

    // "solve" the layout
    if (surplus >= 0) {
        distribute_positive_surplus(surplus, batches);
    }
    else {
        distribute_negative_surplus(surplus, batches);
    }

    float x_offset = m_padding.left;
    for (size_t i = 0; i < m_items.size(); ++i) {
        std::shared_ptr<LayoutItem> child = _get_child(m_items.at(i)); // TODO: this `handle` handling is tedious
        const ItemAdapter& adapter = adapters[i];
        assert(adapter.result >= 0.f);
        const std::pair<float, float> width_to_height = child->get_claim().get_width_to_height();
        float item_height;
        if(width_to_height.first == 0.f){
            item_height = available_height;
        } else {
            item_height = min(available_height, adapter.result / width_to_height.first);
        }
        Size2f item_size(adapter.result, item_height);
        _update_item(child, item_size, Transform2::translation({x_offset, m_padding.top}));
        x_offset += adapter.result + m_spacing;
    }
}

// TODO: there is a bug somewhere: when I set the min in green, it is always larger than the rest

} // namespace notf
