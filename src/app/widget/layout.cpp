#include "notf/app/widget/layout.hpp"

#include "notf/meta/breakable_scope.hpp"

#include <map>

NOTF_OPEN_NAMESPACE

// any layout ======================================================================================================= //

AnyLayout::~AnyLayout() = default;

// overlayout ======================================================================================================= //

WidgetClaim OverLayout::calculate_claim(const std::vector<const WidgetClaim*> claims) const {
    WidgetClaim result;
    for (const WidgetClaim* claim : claims) {
        NOTF_ASSERT(claim);
        result.maxed(*claim);
    }
    return result;
}

std::vector<AnyLayout::Placement> OverLayout::update(const std::vector<const WidgetClaim*> claims,
                                                     const Size2f& grant) const {
    const Size2f available_size = {grant.width() - m_padding.get_width(), grant.height() - m_padding.get_height()};

    std::vector<Placement> placements(claims.size());
    for (size_t i = 0; i < claims.size(); ++i) {
        NOTF_ASSERT(claims[i]);
        const WidgetClaim& claim = *claims[i];
        Placement& placement = placements[i];

        // every widget can claim as much of the available size as it wants
        placement.grant = claim.apply(available_size);

        // the placement of the widget depends on how much it size it claimed
        V2f position;
        position.x() = (max(0, available_size.width() - placement.grant.width()) * m_alignment.horizontal.get_value())
                       + m_padding.left;
        position.y() = (max(0, available_size.height() - placement.grant.height()) * m_alignment.vertical.get_value())
                       + m_padding.bottom;
        placement.xform = M3f::translation(std::move(position));
    }
    return placements;
}

// flex layout ====================================================================================================== //

namespace {
namespace flex_layout {

struct FlexSize {
    float main;
    float cross;
};

struct FlexItem {
    float upper_bound;
    float preferred;
    float scale_factor;
    float lower_bound;
};

struct FlexStack {
    uint first_item = 0;
    uint item_count = 0;
    int priority = min_value<int>();
    FlexItem item = {0, 0, 0, 0};
};

/// Distribute a surplus of space among the given FlexItems.
/// We use FlexItems here instead of WidgetClaims, because stacks (thinkg "rows of widgets", if it is a horizontal
/// FlexLayout) are themselves layed out vertically, just like Widgets would.
/// @param surplus  Remaining surplus.
/// @param batches  Lists of FlexItems, keyed by priority.
/// @returns        Remaining surplus, can be zero but never negative.
float distribute_surplus(float surplus, std::map<int, std::vector<FlexItem*>> priority_batches) {
    for (auto batch_it = priority_batches.rbegin(); batch_it != priority_batches.rend(); ++batch_it) {

        // return early, if the surplus has been depleted by a previous batch
        if (surplus <= precision_high<float>()) { return 0; }
        std::vector<FlexItem*>& batch = batch_it->second;

        // first, supply space to all items that are smaller than their preferred size
        NOTF_BREAKABLE_SCOPE {
            float total_deficit = 0;
            float total_scale_factor = 0;
            for (size_t item_index = 0; item_index < batch.size();) {
                FlexItem* item = batch[item_index];
                float item_deficit = item->preferred - item->lower_bound;
                NOTF_ASSERT(item_deficit >= 0);

                // if the item is smaller than their preferred size, add the deficit to the total
                if (item_deficit > precision_low<float>()) {
                    total_deficit += item_deficit;
                    NOTF_ASSERT(item->scale_factor > 0);
                    total_scale_factor += item->scale_factor;
                }

                // if the lower bound is already touching the upper bound, we can safely ignore this item
                else if (item->upper_bound - item->lower_bound <= precision_high<float>()) {
                    batch[item_index] = std::move(batch.back());
                    batch.pop_back();
                    continue;
                }

                ++item_index; // only advance the iterator if we haven't deleted an item
            }
            if (is_zero(total_deficit, precision_low<float>())) { break; }

            set_min(total_deficit, surplus);

            // first, ensure that all items with a deficit lower than their assigned refund only get as much refund
            // as they need to bump up to their preferred size, the remaining refund flows back into the surplus
            float refund_per_scale_factor = 0; // assign zero to silence false warning...
            {
                bool recalulate_refund = true;
                while (recalulate_refund) {
                    recalulate_refund = false;
                    refund_per_scale_factor = total_deficit / total_scale_factor;

                    for (size_t item_index = 0; item_index < batch.size();) {
                        FlexItem* item = batch[item_index];
                        const float item_deficit = item->preferred - item->lower_bound;
                        const float refund = refund_per_scale_factor * item->scale_factor;

                        // if the deficit is lower than the refund, assign the preferred size and redistribute the rest
                        if (!is_zero(item_deficit) && (item_deficit <= refund)) {
                            item->lower_bound = item->preferred;
                            recalulate_refund = true;
                            surplus -= item_deficit;
                            total_deficit -= item_deficit;
                            total_scale_factor -= item->scale_factor;

                            // if the preferred size is also the upper bound, the item won't affect the layout any more
                            if (is_approx(item->lower_bound, item->upper_bound, precision_high<float>())) {
                                batch[item_index] = std::move(batch.back());
                                batch.pop_back();
                                continue;
                            }
                        }
                        ++item_index;
                    }
                }
            }
            if (is_zero(total_deficit, precision_low<float>())) { break; }

            // at this point, we know that for all remaining items in the batch `lower bound < upper bound` and
            // that the total deficit and scale factor only contains those items where `lower bound < preferred`
            for (FlexItem* item : batch) {
                if (item->preferred - item->lower_bound > 0) {
                    item->lower_bound += refund_per_scale_factor * item->scale_factor;
                }
            }

            surplus -= total_deficit;
        } // breakable scope

        if (surplus <= precision_high<float>()) { return 0; }

        // after taking care of all the deficit, all that's left is to distribute the remaining surplus among the
        // items that have not reached their upper bound yet
        {
            float total_scale_factor = 0;
            for (FlexItem* item : batch) {
                NOTF_ASSERT(item->scale_factor > 0);
                total_scale_factor += item->scale_factor;
            }

            // ensure that all items with a capacity lower than their assigned surplus only get as much surplus
            // as they need to bump up to their uppper bound, the remaining surplus flows back into the total surplus
            float surplus_per_scale_factor = 0; // assign zero to silence false warning...
            {
                bool recalculate_surplus = true;
                while (recalculate_surplus) {
                    recalculate_surplus = false;
                    surplus_per_scale_factor = surplus / total_scale_factor;

                    for (size_t item_index = 0; item_index < batch.size();) {
                        FlexItem* item = batch[item_index];
                        const float item_capacity = item->upper_bound - item->preferred;
                        const float item_surplus = surplus_per_scale_factor * item->scale_factor;

                        // if the item's capacity is enough to handle the surplus, it is assigned in the next step
                        if (item_surplus < item_capacity) {
                            ++item_index;
                            continue;
                        }

                        // if the surplus is higher than the item's capacity, assign it now and redistribute the rest
                        item->lower_bound = item->upper_bound;
                        recalculate_surplus = true;
                        surplus -= item_capacity;
                        total_scale_factor -= item->scale_factor;

                        // remove the item from the batch because it cannot grow any further
                        batch[item_index] = std::move(batch.back());
                        batch.pop_back();
                    }
                }
            }
            if (is_zero(surplus, precision_low<float>())) { return 0; }
            if (batch.empty()) { continue; }

            // at this point we know that each item has enough capacity to hold its assigned surplus and that there are
            // only items left that are not already maxed out
            for (FlexItem* item : batch) {
                item->lower_bound += surplus_per_scale_factor * item->scale_factor;
            }
            return 0; // there can be no surplus left
        }
    }
    return surplus;
}

void layout_single_stack(const FlexLayout& layout, const std::vector<const WidgetClaim*> claims,
                         const FlexSize& available, FlexSize offset, std::vector<AnyLayout::Placement>& placements,
                         const size_t first_item, const size_t item_count) {
    NOTF_ASSERT(item_count > 0);
    NOTF_ASSERT(claims.size() == placements.size());
    NOTF_ASSERT(first_item + item_count <= claims.size());

    // create a FlexItem for each child claim in range
    float surplus;
    std::vector<FlexItem> items(item_count);
    {
        float used_space = 0;
        std::map<int, std::vector<FlexItem*>> items_by_priority;
        for (size_t item_index = 0; item_index < item_count; ++item_index) {
            NOTF_ASSERT(claims[item_index + first_item]);
            const WidgetClaim& claim = *claims[item_index + first_item];
            const WidgetClaim::Stretch& stretch = layout.is_horizontal() ? claim.get_horizontal() :
                                                                           claim.get_vertical();
            FlexItem& item = items[item_index];
            item.scale_factor = stretch.get_scale_factor();

            // apply ratio limits to the upper bound
            item.upper_bound = stretch.get_max();
            if (layout.is_horizontal()) {
                if (const Ratioi& width_to_height = claim.get_ratio_limits().get_upper_bound()) {
                    set_min(item.upper_bound, available.cross * width_to_height.as_real());
                }
            } else { // vertical
                if (const Ratioi& width_to_height = claim.get_ratio_limits().get_lower_bound()) {
                    set_min(item.upper_bound, available.cross / width_to_height.as_real());
                }
            }

            // use new upper bound to limit the item's preferred and lower bound
            item.preferred = min(item.upper_bound, stretch.get_preferred());
            item.lower_bound = min(item.upper_bound, stretch.get_min());

            // store the new item
            items_by_priority[stretch.get_priority()].emplace_back(&item);

            // remove the lower bound from the available space right now
            used_space += item.lower_bound;
        }

        // add spacing
        used_space += (item_count - 1) * layout.get_spacing();

        // distribute remaining surplus
        surplus = available.main - used_space;
        if (surplus > 0) { surplus = distribute_surplus(surplus, std::move(items_by_priority)); }
    }

    const bool is_positive = (layout.get_direction() == FlexLayout::Direction::LEFT_TO_RIGHT)
                             || (layout.get_direction() == FlexLayout::Direction::BOTTOM_TO_TOP);
    const bool is_horizontal = layout.is_horizontal();

    // determine values for alignment along the main axis
    float spacing = layout.get_spacing();
    {
        float initial_main_offset = 0;
        switch (layout.get_alignment()) {
        case FlexLayout::Alignment::START: break;
        case FlexLayout::Alignment::END: initial_main_offset = surplus; break;
        case FlexLayout::Alignment::CENTER: initial_main_offset = surplus * 0.5f; break;
        case FlexLayout::Alignment::SPACE_BETWEEN:
            spacing += max(0, item_count > 1 ? surplus / static_cast<float>(item_count - 1) : 0);
            break;
        case FlexLayout::Alignment::SPACE_AROUND:
            initial_main_offset = surplus / static_cast<float>(item_count * 2);
            spacing += max(0, initial_main_offset * 2);
            break;
        case FlexLayout::Alignment::SPACE_EQUAL:
            initial_main_offset = surplus / static_cast<float>(item_count + 1);
            spacing += max(0, initial_main_offset);
            break;
        }
        offset.main += initial_main_offset * (is_positive ? 1 : -1);
    }

    // apply the layout
    NOTF_ASSERT(first_item + items.size() <= claims.size());
    for (size_t index = 0; index < items.size(); ++index) {
        const WidgetClaim& claim = *claims[first_item + index];
        const WidgetClaim::Stretch& cross_stretch = is_horizontal ? claim.get_vertical() : claim.get_horizontal();
        const FlexItem& item = items[index];
        NOTF_ASSERT(item.lower_bound >= 0);
        NOTF_ASSERT(item.lower_bound <= item.upper_bound);

        const FlexSize item_size{item.lower_bound, min(cross_stretch.get_max(), available.cross)};

        float cross_align_offset;
        switch (layout.get_cross_alignment()) {
        case FlexLayout::Alignment::START:
            cross_align_offset = is_horizontal ? available.cross - item_size.cross : 0;
            break;
        case FlexLayout::Alignment::END:
            cross_align_offset = is_horizontal ? 0 : available.cross - item_size.cross;
            break;
        case FlexLayout::Alignment::CENTER:
        case FlexLayout::Alignment::SPACE_BETWEEN:
        case FlexLayout::Alignment::SPACE_AROUND:
        case FlexLayout::Alignment::SPACE_EQUAL: cross_align_offset = (available.cross - item_size.cross) * 0.5f;
        }

        AnyLayout::Placement& placement = placements[first_item + index];
        if (is_horizontal) {
            placement.grant = claim.apply(Size2f(item_size.main, item_size.cross));
            placement.xform = M3f::translation(is_positive ? offset.main : offset.main - item_size.main,
                                               offset.cross + cross_align_offset);
        } else {
            placement.grant = claim.apply(Size2f(item_size.cross, item_size.main));
            placement.xform = M3f::translation(offset.cross + cross_align_offset,
                                               is_positive ? offset.main : offset.main - item_size.main);
        }

        offset.main += (item.lower_bound + spacing) * (is_positive ? 1 : -1);
    }
}

void layout_multi_stack(const FlexLayout& layout, const std::vector<const WidgetClaim*> claims,
                        const FlexSize& available, FlexSize offset, std::vector<AnyLayout::Placement>& placements) {

    std::vector<FlexStack> stacks;
    float cross_surplus;
    { // fill the child claims into stacks
        FlexStack current_stack;

        FlexSize used_space = {0, 0};
        for (uint claim_index = 0, end = narrow_cast<uint>(claims.size()); claim_index < end; ++claim_index) {

            // get the claim's main- and cross stretches
            NOTF_ASSERT(claims[claim_index]);
            const WidgetClaim& claim = *claims[claim_index];
            const WidgetClaim::Stretch& main_stretch = layout.is_horizontal() ? claim.get_horizontal() :
                                                                                claim.get_vertical();
            const WidgetClaim::Stretch& cross_stretch = layout.is_horizontal() ? claim.get_vertical() :
                                                                                 claim.get_horizontal();

            // add the claim's preferred value to the used space
            used_space.main += main_stretch.get_preferred() + layout.get_spacing();

            // if the child doesn't fit on the current stack anymore, open a new one
            if ((used_space.main > available.main) && current_stack.item_count) {
                used_space.cross += current_stack.item.lower_bound;
                stacks.emplace_back(std::move(current_stack));

                current_stack = {}; // re-using moved container, see http://stackoverflow.com/a/9168917/3444217
                current_stack.first_item = claim_index;

                used_space.main = 0;
            }

            // if there is enough space, add the child to the current stack
            ++current_stack.item_count;
            set_max(current_stack.priority, cross_stretch.get_priority());
            set_max(current_stack.item.upper_bound, cross_stretch.get_max());
            set_max(current_stack.item.preferred, cross_stretch.get_preferred());
            set_max(current_stack.item.scale_factor, cross_stretch.get_scale_factor());
            set_max(current_stack.item.lower_bound, cross_stretch.get_min());
        }

        // add the last, partially filled stack
        used_space.cross += current_stack.item.lower_bound;
        stacks.emplace_back(std::move(current_stack));

        // add cross spacing
        used_space.cross += (stacks.size() - 1) * layout.get_cross_spacing();
        cross_surplus = max(0, available.cross - used_space.cross);
    }

    // distribute cross surplus among the stacks
    if (cross_surplus > 0) {

        // we need to order all stacks by priority and use a map since we don't know how many priorities there are
        std::map<int, std::vector<FlexItem*>> stacks_by_priority;
        for (FlexStack& stack : stacks) {
            stacks_by_priority[stack.priority].emplace_back(&stack.item);
        }

        // the cross directional layout of stacks is itself a FlexLayout
        cross_surplus = distribute_surplus(cross_surplus, std::move(stacks_by_priority));
    }

    // determine values for alignment along the cross axis
    float cross_spacing = layout.get_cross_spacing();
    {
        float cross_start = 0;
        const size_t item_count = stacks.size();
        switch (layout.get_stack_alignment()) {
        case FlexLayout::Alignment::START: break;
        case FlexLayout::Alignment::END: cross_start = cross_surplus; break;
        case FlexLayout::Alignment::CENTER: cross_start = cross_surplus * 0.5f; break;
        case FlexLayout::Alignment::SPACE_BETWEEN:
            cross_spacing += item_count > 1 ? cross_surplus / static_cast<float>(item_count - 1) : 0;
            break;
        case FlexLayout::Alignment::SPACE_AROUND:
            cross_start = cross_surplus / static_cast<float>(item_count * 2);
            cross_spacing += cross_start * 2;
            break;
        case FlexLayout::Alignment::SPACE_EQUAL:
            cross_start = cross_surplus / static_cast<float>(item_count + 1);
            cross_spacing += cross_start;
            break;
        }
        offset.cross += cross_start;
    }

    // layout all stacks in order
    const bool is_reverse = layout.get_wrap() == FlexLayout::Wrap::REVERSE;
    for (size_t stack_index = 0, end = stacks.size(); stack_index < end; ++stack_index) {
        const FlexStack& stack = stacks[is_reverse ? stacks.size() - (stack_index + 1) : stack_index];
        const FlexSize stack_size = {available.main, stack.item.lower_bound};

        if (layout.is_vertical()) {
            layout_single_stack(layout, claims, stack_size, offset, placements, stack.first_item, stack.item_count);
            offset.cross += stack_size.cross + cross_spacing;
        } else {
            offset.cross -= stack_size.cross;
            layout_single_stack(layout, claims, stack_size, offset, placements, stack.first_item, stack.item_count);
            offset.cross -= cross_spacing;
        }
    }
}

} // namespace flex_layout
} // namespace

WidgetClaim FlexLayout::calculate_claim(const std::vector<const WidgetClaim*> claims) const {
    // wrapping layouts adapt to whatever space is offered
    if (is_wrapping()) { return {}; }

    WidgetClaim result;
    if (is_horizontal()) {
        for (const WidgetClaim* claim : claims) {
            result.add_horizontal(*claim);
        }
        if (!claims.empty()) { result.get_horizontal().grow_by((claims.size() - 1) * m_spacing); }
    } else {
        NOTF_ASSERT(is_vertical());
        for (const WidgetClaim* claim : claims) {
            result.add_vertical(*claim);
        }
        if (!claims.empty()) { result.get_vertical().grow_by((claims.size() - 1) * m_spacing); }
    }
    return result;
}

std::vector<AnyLayout::Placement> FlexLayout::update(const std::vector<const WidgetClaim*> claims,
                                                     const Size2f& grant) const {
    using namespace flex_layout;

    // early out if there are no items to lay out
    if (claims.empty()) { return {}; }

    // determine available space to fit the layout into
    FlexSize available_space = [&] {
        FlexSize result;
        const Size2f available = grant - get_padding().get_size();
        result.main = is_horizontal() ? available.width() : available.height();
        result.cross = is_horizontal() ? available.height() : available.width();
        return result;
    }();

    // determine the offset of the first stack
    FlexSize offset;
    switch (m_direction) {
    case FlexLayout::Direction::LEFT_TO_RIGHT:
        offset.main = m_padding.left;
        offset.cross = m_wrap == FlexLayout::Wrap::WRAP ? grant.height() - m_padding.top : m_padding.bottom;
        break;
    case FlexLayout::Direction::RIGHT_TO_LEFT:
        offset.main = grant.width() - m_padding.right;
        offset.cross = m_wrap == FlexLayout::Wrap::WRAP ? grant.height() - m_padding.top : m_padding.bottom;
        break;
    case FlexLayout::Direction::TOP_TO_BOTTOM:
        offset.main = grant.height() - m_padding.top;
        offset.cross = m_wrap == FlexLayout::Wrap::WRAP ? m_padding.left : grant.width() - m_padding.right;
        break;
    case FlexLayout::Direction::BOTTOM_TO_TOP:
        offset.main = m_padding.bottom;
        offset.cross = m_wrap == FlexLayout::Wrap::WRAP ? m_padding.left : grant.width() - m_padding.right;
        break;
    }

    std::vector<Placement> placements(claims.size());
    if (is_wrapping()) {
        layout_multi_stack(*this, claims, available_space, std::move(offset), placements);
    } else {
        layout_single_stack(*this, claims, available_space, std::move(offset), placements, 0, claims.size());
    }
    return placements;
}

NOTF_CLOSE_NAMESPACE
