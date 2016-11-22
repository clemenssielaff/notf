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

    float lower_bound;
    float upper_bound;

    /** Base size. */
    float base;

    /** Scale factor. */
    float scale_factor;

    /** Calculated size. */
    float result;
};

void distribute_surplus(float surplus, std::map<int, std::set<ItemAdapter*>>& batches)
{
    for (auto batch_it = batches.rbegin(); batch_it != batches.rend(); ++batch_it) {
        std::set<ItemAdapter*>& batch = batch_it->second;
        while (!batch.empty()) {

            // do nothing, if the surplus has been depleted
            if (approx(surplus, 0.f)) {
                batch.clear();
                continue;
            }

            float total_scale_factor = 0.;
            float total_base = 0.f;
            // TODO: what does the "base" or "preferred" size really mean? isn't that just like the scale factor?
            /* Not if items grow to their preferred size before the rest of the surplus is distributed
             * Meaning, that if two widgets have their preferred size and one is lower and there is a little bit of
             * surplus left, then the one whose size is not at the preferred will get it first.
             * That makes sense :-)
             * And we can change the name back to "preferred"
             */
            for (const ItemAdapter* item : batch) {
                assert(item->scale_factor > 0.f);
                total_scale_factor += item->scale_factor;
                total_base += item->base;
            }
            float surplus = surplus - total_base;
            assert(total_scale_factor > 0.f);
            float surplus_per_scale_factor = surplus / total_scale_factor;

            // if the size is higher than the max of any item, it's surplus must be distributed among the batch
            bool surplus_fits_in_batch = true;
            for (const ItemAdapter* item : batch) {
                const float base_offset = item->scale_factor * surplus_per_scale_factor;
                if (item->lower_bound > item->base + base_offset) {
                    surplus -= item->lower_bound;
                    surplus_fits_in_batch = false;
                    auto mutable_item = const_cast<ItemAdapter*>(item);
                    batch.erase(mutable_item);
                    mutable_item->result = item->lower_bound;
                }
                else if (item->upper_bound < item->base + base_offset) {
                    surplus -= item->upper_bound;
                    surplus_fits_in_batch = false;
                    auto mutable_item = const_cast<ItemAdapter*>(item);
                    batch.erase(mutable_item);
                    mutable_item->result = item->upper_bound;
                }
            }

            // if all sizes fit, we can use up the surplus with this batch
            if (surplus_fits_in_batch) {
                for (const ItemAdapter* item : batch) {
                    const float base_offset = item->scale_factor * surplus_per_scale_factor;

                    auto mutable_item = const_cast<ItemAdapter*>(item);
                    batch.erase(mutable_item);
                    mutable_item->result = item->base + base_offset;
                }
                assert(batch.empty());
                surplus = 0.f;
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
        const std::pair<float, float> width_to_height = claim.get_width_to_height();
        if (width_to_height.first > 0.f) {
            adapters[i].lower_bound = min(direction.get_min(), available_height / width_to_height.first);
            adapters[i].upper_bound = min(direction.get_max(), available_height / width_to_height.second);
        }
        else {
            adapters[i].lower_bound = direction.get_min();
            adapters[i].upper_bound = direction.get_max();
        }
        adapters[i].base = direction.get_base();
        adapters[i].scale_factor = direction.get_scale_factor();
        adapters[i].result = direction.get_min();
        total_min += direction.get_min();
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
        assert(adapter.result >= adapter.lower_bound);
        assert(adapter.result <= adapter.upper_bound);

        const std::pair<float, float> width_to_height = child->get_claim().get_width_to_height();
        Size2f item_size(adapter.result, available_height);
        if (width_to_height.first > 0.f) {
            item_size.height = min(available_height, adapter.result / width_to_height.first);
        }
        _update_item(child, item_size, Transform2::translation({x_offset, m_padding.top}));
        x_offset += adapter.result + m_spacing;
    }
#ifdef _DEBUG
    if (!approx(x_offset, available_width - m_padding.right + m_spacing)) {
        log_warning << "SOFT ASSERT";
    }
#endif

    // TODO: bug with a very narrow window that is stretched vertically - the red box "jumps" in width depending on the height of the window and affects the blue
}

} // namespace notf
